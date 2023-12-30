#include <string.h>
#include <esp_spiffs.h>
#include <esp_log.h>

#include "spiffs_initialise.h"

static const char *TAG = "SPIFFS";

void spiffs_initialise()
{
  esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true};

  esp_err_t ret = esp_vfs_spiffs_register(&conf);
  if (ret == ESP_OK)
  {
    ESP_LOGI(TAG, "Sucessfully initialised");
  }
  else
  {
    ESP_LOGE(TAG, "Failed to initialize (%s)", esp_err_to_name(ret));
  }

  size_t total = 0, used = 0;
  ret = esp_spiffs_info(conf.partition_label, &total, &used);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s). Formatting...", esp_err_to_name(ret));
    esp_spiffs_format(conf.partition_label);
    return;
  }
  else
  {
    ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
  }

  // Check consistency of reported partiton size info.
  if (used > total)
  {
    ESP_LOGW(TAG, "Number of used bytes cannot be larger than total. Performing SPIFFS_check().");
    ret = esp_spiffs_check(conf.partition_label);
    // Could be also used to mend broken files, to clean unreferenced pages, etc.
    // More info at https://github.com/pellepl/spiffs/wiki/FAQ#powerlosses-contd-when-should-i-run-spiffs_check
    if (ret != ESP_OK)
    {
      ESP_LOGE(TAG, "SPIFFS_check() failed (%s)", esp_err_to_name(ret));
      return;
    }
    else
    {
      ESP_LOGI(TAG, "SPIFFS_check() successful");
    }
  }
}