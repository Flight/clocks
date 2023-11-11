#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs.h>
#include <nvs_flash.h>
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

extern const uint8_t server_cert_pem_start[] asm("_binary_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_cert_pem_end");

static const uint8_t TIME_BEFORE_UPDATE_CHECK_SECS = 10;

static size_t stored_hash_size = HASH_LEN;
static uint8_t sha_256_current[HASH_LEN] = {0};
static uint8_t sha_256_stored[HASH_LEN] = {0};
static nvs_handle_t ota_storage_handle;

static void print_sha256(const uint8_t *image_hash, const char *label)
{
  char hash_print[HASH_LEN * 2 + 1];
  hash_print[HASH_LEN * 2] = 0;
  for (int i = 0; i < HASH_LEN; ++i)
  {
    sprintf(&hash_print[i * 2], "%02x", image_hash[i]);
  }
  ESP_LOGI(TAG, "%s %s", label, hash_print);
}

static void check_current_firmware(void)
{
  ESP_LOGI(TAG, "Checkng current firmware...");
  nvs_open("storage", NVS_READWRITE, &ota_storage_handle);

  // Read the stored hash
  esp_err_t err = nvs_get_blob(ota_storage_handle, "firmware_hash", sha_256_stored, &stored_hash_size);
  if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
  {
    ESP_LOGE(TAG, "Failed to read hash from NVS: %s", esp_err_to_name(err));
    nvs_close(ota_storage_handle);
    vTaskDelete(NULL);
    return;
  }

  print_sha256(sha_256_stored, "Stored hash: ");

  // Get the SHA-256 of the current firmware
  esp_partition_get_sha256(esp_ota_get_running_partition(), sha_256_current);
  print_sha256(sha_256_current, "Current hash: ");

  // If the stored hash is the same size and matches the current one
  // We can mark it as valid and cancel the rollback
  if (err == ESP_ERR_NVS_NOT_FOUND || (stored_hash_size == HASH_LEN && memcmp(sha_256_current, sha_256_stored, HASH_LEN) == 0))
  {
    ESP_LOGI(TAG, "Diagnostics completed successfully! Marking partition as valid and cancelling rollback.");
    esp_ota_mark_app_valid_cancel_rollback();
    esp_ota_erase_last_boot_app_partition();
  }
}

static void update_firmware()
{
  esp_http_client_config_t config = {
      .url = FIRMWARE_UPGRADE_URL,
      .cert_pem = (char *)server_cert_pem_start,
      .keep_alive_enable = true,
  };

  esp_https_ota_config_t ota_config = {
      .http_config = &config,
  };

  esp_err_t err = esp_https_ota(&ota_config);

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "OTA Update failed!");
    nvs_close(ota_storage_handle);
    vTaskDelete(NULL);
    return;
  }

  ESP_LOGI(TAG, "OTA Update successful!");

  // If update was successful, store the new hash
  esp_partition_get_sha256(esp_ota_get_running_partition(), sha_256_current);
  print_sha256(sha_256_current, "New hash: ");
  err = nvs_set_blob(ota_storage_handle, "firmware_hash", sha_256_current, HASH_LEN);
  nvs_commit(ota_storage_handle); // Commit changes to NVS
  ESP_LOGI(TAG, "Stored new firmware hash in NVS.");

  ESP_LOGI(TAG, "Restarting to the new firmware...");
  ESP_LOGW(TAG, "Please stop the web server to prevent the update loop!");
  nvs_close(ota_storage_handle);
  esp_restart();
}

void ota_update_task(void *pvParameter)
{
  check_current_firmware();

  ESP_LOGI(TAG, "Waiting for Wi-Fi");
  xEventGroupWaitBits(global_event_group, IS_WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

  vTaskDelay(1000 * TIME_BEFORE_UPDATE_CHECK_SECS / portTICK_PERIOD_MS);

  nvs_close(ota_storage_handle);
  vTaskDelete(NULL);

  update_firmware();
}
