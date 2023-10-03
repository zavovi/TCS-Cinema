/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include "bsp/esp-bsp.h"
#include "lvgl.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "esp_h264_enc.h"

static const char *TAG = "EXAMPLE";

extern void example_lvgl_demo_ui(lv_obj_t *scr);

static void app_show_video_task(void *arg)
{
	int one_image_size = 0;
     esp_h264_err_t ret = ESP_H264_ERR_OK;
     esp_h264_enc_t handle = NULL;
     esp_h264_enc_cfg_t cfg = DEFAULT_H264_ENCODER_CONFIG();
     esp_h264_enc_frame_t out_frame = { 0 };
     esp_h264_raw_frame_t in_frame = { 0 };
     int frame_count = 0;
     int ret_w = 0;
	 
	bsp_display_lock(0);
    lv_obj_t *camera_canvas = lv_canvas_create(lv_scr_act());
    assert(camera_canvas);
    lv_obj_center(camera_canvas);
    bsp_display_unlock();
	 
     /*FILE *out = fopen(BSP_SD_MOUNT_POINT"/movie.h264", "wb+");
     if (out == NULL) {
         printf("Output file cann't open \r\n");
         return;
     }*/
	bsp_display_lock(0);
    FILE *in = fopen(BSP_SD_MOUNT_POINT"/movie.mp4", "rb");
    bsp_display_unlock();
     if (in == NULL) {
         printf("Input file cann't open \r\n");
         goto h264_example_exit;
     }
     cfg.fps = 24;
     cfg.height = 232;
     cfg.width = 320;
     cfg.pic_type = ESP_H264_RAW_FMT_I420;
     one_image_size = cfg.height * cfg.width * 1.5; // 1.5 : Pixel is 1.5 on ESP_H264_RAW_FMT_I420.
     in_frame.raw_data.buffer = (uint8_t *)heap_caps_aligned_alloc(16, one_image_size, MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
     if (in_frame.raw_data.buffer == NULL) {
         printf("Allocation memory failed \r\n");
         goto h264_example_exit;
     }
     ret = esp_h264_enc_open(&cfg, &handle);
     if (ret != ESP_H264_ERR_OK) {
         printf("Open failed. ret %d, handle %p \r\n", ret, handle);
         goto h264_example_exit;
     }
	 
	 ESP_LOGW(TAG, "test 1 ");
	 
     while(1) {
         ret_w = fread(in_frame.raw_data.buffer, 1, one_image_size, in);
         if (ret_w != one_image_size) {
             printf("Encoder finished, ret %d \r\n", ret_w);
             goto h264_example_exit;
         }
         in_frame.pts = frame_count * (1000 / cfg.fps);
         ret = esp_h264_enc_process(handle, &in_frame, &out_frame);
         if (ret != ESP_H264_ERR_OK) {
             printf("Process failed. ret %d \r\n", ret);
             goto h264_example_exit;
         }
         for (size_t layer = 0; layer < out_frame.layer_num; layer++) {
	 
			ESP_LOGW(TAG, "test 2 ");
			 
            bsp_display_lock(0);
            lv_canvas_set_buffer(camera_canvas, out_frame.layer_data[layer].buffer, cfg.width, cfg.height, LV_IMG_CF_TRUE_COLOR);
            bsp_display_unlock();
			
             /*ret_w = fwrite(out_frame.layer_data[layer].buffer, 1, out_frame.layer_data[layer].len, out);
             if (ret_w != out_frame.layer_data[layer].len) {
                 printf("fwrite happened error, ret %d \r\n", ret_w);
                 goto h264_example_exit;
             }*/
         }
         frame_count++;
     }
 h264_example_exit:
     if (in) {
         fclose(in);
     }
     /*if (out) {
         fclose(out);
     }*/
     if (in_frame.raw_data.buffer) {
         heap_caps_free(in_frame.raw_data.buffer);
         in_frame.raw_data.buffer = NULL;
     }
     esp_h264_enc_close(handle);

    /* Close task */
    vTaskDelete( NULL );
}

void app_main(void)
{
	bsp_i2c_init();
	bsp_sdcard_mount();
    bsp_display_start();
	
    xTaskCreate(app_show_video_task, "video task", 4096, NULL, 4, NULL);

    /*ESP_LOGI("example", "Display LVGL animation");
    bsp_display_lock(0);
    lv_obj_t *scr = lv_disp_get_scr_act(NULL);
    example_lvgl_demo_ui(scr);

    bsp_display_unlock();
    bsp_display_backlight_on();*/
}
