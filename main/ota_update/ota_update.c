#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs.h>
#include <esp_system.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_ota_ops.h>
#include <esp_http_client.h>
#include <esp_https_ota.h>
#include <esp_app_format.h>
#include <string.h>
#include <mbedtls/sha256.h>

#include "global_event_group.h"

#define FIRMWARE_UPGRADE_URL CONFIG_FIRMWARE_UPGRADE_URL
#define HASH_LEN 32
#define OTA_BUF_SIZE 256

static const char *TAG = "OTA FW Update";

static const char *NVS_STORAGE_NAMESPACE = "system_info";
static const char *NVS_FIRMWARE_HASH_KEY = "firmware_hash";

extern const uint8_t server_cert_pem_start[] asm("_binary_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_cert_pem_end");

static const uint8_t TIME_BEFORE_UPDATE_CHECK_SECS = 10;

static size_t stored_hash_size = HASH_LEN;
static uint8_t sha_256_current[HASH_LEN] = {0};
static uint8_t sha_256_boot[HASH_LEN] = {0};
static uint8_t sha_256_stored[HASH_LEN] = {0};
static nvs_handle_t ota_storage_handle;

static void print_sha256(const uint8_t *image_hash, const char *label)
{
  char hash_print[HASH_LEN * 2 + 1];
  hash_print[HASH_LEN * 2] = 0;
  for (uint8_t i = 0; i < HASH_LEN; ++i)
  {
    sprintf(&hash_print[i * 2], "%02x", image_hash[i]);
  }
  ESP_LOGI(TAG, "%s %s", label, hash_print);
}

static void check_current_firmware(void)
{
  ESP_LOGI(TAG, "Checking current firmware...");
  const esp_partition_t *running_partition = esp_ota_get_running_partition();

  nvs_open(NVS_STORAGE_NAMESPACE, NVS_READWRITE, &ota_storage_handle);

  // Read the stored hash
  esp_err_t err = nvs_get_blob(ota_storage_handle, NVS_FIRMWARE_HASH_KEY, &sha_256_stored, &stored_hash_size);
  if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
  {
    ESP_LOGE(TAG, "Failed to read hash from NVS: %s", esp_err_to_name(err));
    return;
  }

  print_sha256(sha_256_stored, "Stored hash: ");

  // Get the SHA-256 of the current firmware
  esp_partition_get_sha256(running_partition, sha_256_current);
  print_sha256(sha_256_current, "Current hash: ");

  // Check if the running partition is the factory partition
  bool is_factory_partition = running_partition->type == ESP_PARTITION_TYPE_APP &&
                              running_partition->subtype == ESP_PARTITION_SUBTYPE_APP_FACTORY;

  if (is_factory_partition)
  {
    ESP_LOGI(TAG, "The current partition is factory one.");
    // If the stored hash is not the same as the factory one, save the current hash
    if (stored_hash_size != HASH_LEN || memcmp(sha_256_current, sha_256_stored, HASH_LEN) != 0)
    {
      nvs_set_blob(ota_storage_handle, NVS_FIRMWARE_HASH_KEY, &sha_256_current, HASH_LEN);
      nvs_commit(ota_storage_handle);
      ESP_LOGI(TAG, "Stored new firmware hash in NVS.");
    }
    return;
  }

  // If the stored hash is the same size and matches the current one
  // We can mark it as valid and cancel the rollback
  if (err == ESP_ERR_NVS_NOT_FOUND || (stored_hash_size == HASH_LEN && memcmp(sha_256_current, sha_256_stored, HASH_LEN) == 0))
  {
    esp_ota_img_states_t ota_state;
    esp_ota_get_state_partition(running_partition, &ota_state);

    if (ota_state == ESP_OTA_IMG_PENDING_VERIFY)
    {
      ESP_LOGI(TAG, "Diagnostics completed successfully! Marking partition as valid and cancelling rollback.");
      if (esp_ota_mark_app_valid_cancel_rollback() == ESP_OK)
      {
        ESP_LOGI(TAG, "Rollback cancelled successfully");
        esp_ota_erase_last_boot_app_partition();
        ESP_LOGI(TAG, "Erased old partition");
      }
      else
      {
        ESP_LOGE(TAG, "Failed to cancel rollback");
      }
    }
  }
}

static void check_for_updates()
{
  esp_http_client_config_t config = {
      .url = FIRMWARE_UPGRADE_URL,
      .cert_pem = (char *)server_cert_pem_start,
  };
  esp_https_ota_config_t ota_config = {
      .http_config = &config,
  };
  esp_err_t err = esp_https_ota(&ota_config);

  if (err != ESP_OK)
  {
    ESP_LOGW(TAG, "OTA Update failed!");
    return;
  }

  ESP_LOGI(TAG, "OTA Update successful!");

  // If update was successful, store the new hash
  esp_partition_get_sha256(esp_ota_get_boot_partition(), sha_256_boot);
  print_sha256(sha_256_boot, "New hash: ");
  nvs_set_blob(ota_storage_handle, NVS_FIRMWARE_HASH_KEY, &sha_256_boot, HASH_LEN);
  nvs_commit(ota_storage_handle);
  nvs_close(ota_storage_handle);
  ESP_LOGI(TAG, "Stored new firmware hash in NVS.");
  ESP_LOGI(TAG, "Restarting to the new firmware...");
  ESP_LOGW(TAG, "Please stop the web server to prevent the update loop!");
  esp_restart();
}

void ota_update_task(void *pvParameter)
{
  ESP_LOGI(TAG, "Waiting for Wi-Fi");
  xEventGroupWaitBits(global_event_group, IS_WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

  vTaskDelay(1000 * TIME_BEFORE_UPDATE_CHECK_SECS / portTICK_PERIOD_MS);

  check_current_firmware();
  check_for_updates();

  ESP_LOGI(TAG, "Cleaning-up OTA update task.");
  nvs_close(ota_storage_handle);
  vTaskDelete(NULL);
}
