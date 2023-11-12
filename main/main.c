/**
 * @file main.c
 * @author Trương Quốc Ánh (you@domain.com)
 * @brief Save information to SD card
 * @version 0.1
 * @date 2023-11-12
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <esp_system.h>
#include <nvs_flash.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"

#define BUTTON_PIN 3
static void button_handler (void *arg);

/**
 * @brief Initialize the SD card configuration.
 * @note SDMMC: SD MultiMediaCards
 * @return esp_err_t 
 */
static esp_err_t init_sd_card (void)
{
    /**
     * @brief Default sdmmc_host_t structure initializer for SDMMC peripheral Uses SDMMC peripheral, with 4-bit mode enabled, and max frequency set to 20MHz
     * @note This variable identify communication's settings to SD Card
     */
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT(); /** Macro defining default configuration of SDMMC host slot */
    /** this code line use to config stucture of Mount (gắn kết) file system FAT's process */
    esp_vfs_fat_sdmmc_mount_config_t mount_config =
    {
        .format_if_mount_failed = false, /** Do not reformat  SD card whenever having problem in mount processing */
        .max_files = 3, /** Max number of opened files in the same time */
    };
    sdmmc_card_t *card;
    /**
     * @brief After config the structure of FAT in code line upper, this line will initialize and mount FAT file
     * @param 1: Link where we want to mount file FAT into
     * @param 2: pointer of "sdmmc_host_t" configured. This variable identify communication's settings to SD Card
     * @param 3: pointer of "sdmmc_slot_config_t" configured. This variable contains settings related to the SD Card slot
     * @param 4: pointer of "sdmmc_card_t". When this function succeed, the data of this function will be saved in this variable.
     */
    esp_err_t err = esp_vfs_fat_sdmmc_mount("/SD card", &host, &slot_config, &slot_config, &card);
    if (err != ESP_OK)
    {
        return err;
    }
    return ESP_OK;
}
/** Initialize Interrupt Service Routine.  */
static esp_err_t init_button_isr (void)
{
    gpio_config_t io_conf;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << BUTTON_PIN);    /** GPIO pin: set with bit mask, each bit maps to a GPIO */ /**I don't really understand this line :))) */
    io_conf.intr_type = GPIO_INTR_POSEDGE;          /** Interrupt whenever the button rising */
    io_conf.pull_up_en = 1;                         /** GPIO Pull-up */  
    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK)
    {
        return err;
    }
    return gpio_isr_handler_add(BUTTON_PIN, button_handler, NULL);
}

/** Save information to SD care whenever Interrupt occur */

static void save_inf(void *arg)
{
    char str[] = "Hello World!!!";
    FILE *file = fopen("inf_name.text", "w");
    if (file == NULL)
    {
        printf(" err: fopen failed\n");
    }
    else 
    {
        fwrite(str, 1, sizeof(str), file);
        fclose(file);
    }
    vTaskDelete(NULL);
    printf(" Finished Save Information\n");
}

static TickType_t next = 0;
const TickType_t period = 2000/ portTICK_PERIOD_MS;

/** Execution function when interrupt occur   */
static void IRAM_ATTR button_handler (void *arg)
{
    TickType_t now = xTaskGetTickCountFromISR();
    if (now > next)
    {
        xTaskCreate(save_inf, "Save infor", 1024, NULL, 15, NULL);
    }
    /** Use this way to make sure we don't save information to SD card continuous, at least we have 2 seconds waiting for SD card handle its data */
    next = now + period;
    
}

void app_main()
{
    esp_err_t err;
    err = init_sd_card();
    if (err != ESP_OK)
        {
            printf(" err: %s", esp_err_to_name(err));
            return;
        }
    err = init_button_isr();
    /**
     * @brief Value return of the init faction is the error code of gpio_isr_handler_add. If we add ISR handler to Button Pin successfully, we will skip 
     * this code
     * 
     */
    if (err != ESP_OK)
    {
        printf("err: %s\n", esp_err_to_name(err));
        return;
    }
    printf(" Everything Done!!");

}