// SPDX-License-Identifier: MIT

/**
 * @file i2s.h
 * @author BambooMaster (https://misskey.hakoniwa-project.com/@BambooMaster)
 * @brief pico-i2s-pio
 * @version 0.4
 * @date 2025-05-05
 * 
 */

 #ifndef I2S_H
 #define I2S_H
#include <stdint.h>
 #include "hardware/pio.h"
 
 //#define I2S_START_LEVEL     (I2S_BUF_DEPTH / 4)
 //#define I2S_TARGET_LEVEL    (I2S_BUF_DEPTH * 3 / 4)
#define I2S_BUF_DEPTH   32
#define I2S_START_LEVEL     12
#define I2S_TARGET_LEVEL    16
 //#define I2S_TARGET_LEVEL_MIN_US    1200
 //#define I2S_TARGET_LEVEL_MAX_US    1600
#define I2S_TARGET_LEVEL_MIN_US    12000
#define I2S_TARGET_LEVEL_MAX_US    20000
 #define I2S_DATA_LEN    ((48 + 8) * 8 * 2)

//-----------------------------------------------------------------------------
// Optional: device-side clock trimming (rate control).
//
// This project already uses USB Audio feedback (tud_audio_fb_set()) to regulate
// the host send rate. Enabling *both* USB feedback and device clock trim creates
// two competing control loops that can slowly oscillate (periodic pops / fuzz).
// Default is OFF.
//-----------------------------------------------------------------------------
#ifndef I2S_ENABLE_CLOCK_TRIM
#define I2S_ENABLE_CLOCK_TRIM 0
#endif
 
 typedef enum {
     MODE_I2S,
     MODE_PT8211,
     MODE_EXDF,
     MODE_I2S_DUAL,
     MODE_PT8211_DUAL
 } I2S_MODE;
 
 typedef enum {
     CLOCK_MODE_DEFAULT,
     CLOCK_MODE_LOW_JITTER_LOW,
     CLOCK_MODE_LOW_JITTER,
     CLOCK_MODE_LOW_JITTER_OC,
     CLOCK_MODE_EXTERNAL
 } CLOCK_MODE;
 
 /**
  * @brief 再生状態の切り替わりを通知する関数の型
  * 
  * @param state 再生状態 true:再生開始 false:再生停止
  */
 typedef void (*ExternalFunction)(bool state);
 
 /**
  * @brief core1のmain関数の型
  * 
  */
 typedef void (*Core1MainFunction)(void);
 
 /**
  * @brief i2sの出力ピンを設定する
  * 
  * @param data_pin data出力ピン
  * @param clock_pin_base LRCLK出力ピン
  * @param mclk_pin_pin LRCLK出力ピン
  * @note BCLK=clock_pin_base+1
  * @note MODE_EXDFの場合、DOUTL = data_pin, DOUTR = data_pin + 1, WCK=clock_pin_base, BCK=clock_pin_base+1 MCLK=clock_pin_base+2
  */
 void i2s_mclk_set_pin(uint data_pin, uint clock_pin_base, uint mclk_pin);
 
 /**
  * @brief i2sの設定を行う
  * 
  * @param pio i2sに使用するpio pio0 or pio1
  * @param sm i2sに使用するsm0~2 (mclkはsm+1を使う)
  * @param dma_ch i2sに使用するdmaチャンネル
  * @param use_core1 pioのFIFOへデータを送る処理をcore1で行うかどうか
  * @param clock_mode クロックモードの選択 (CLOCK_MODE_DEFAULT, CLOCK_MODE_LOW_JITTER, CLOCK_MODE_LOW_JITTER_OC, CLOCK_MODE_EXTERNAL)
  * @param mode 出力するフォーマットを選択する (MODE_I2S, MODE_PT8211, MODE_EXDF, MODE_I2S_DUAL, MODE_PT8211_DUAL)
  * @note lowジッタモードを使用する場合はuart,i2s,spi設定よりも先に呼び出す
  * @note MODE_PT8211はBCLK32fsのlsbj16,MCLKなし
  */
 void i2s_mclk_set_config(PIO pio, uint sm, int dma_ch, bool use_core1, CLOCK_MODE clock_mode, I2S_MODE mode);
 
 /**
  * @brief i2sの初期化を行う
  * 
  * @param audio_clock サンプリング周波数
  * @note 呼び出してすぐにi2sの出力が開始される
  */
 void i2s_mclk_init(uint32_t audio_clock);
 
 /**
  * @brief i2sの周波数を変更する
  * 
  * @param audio_clock サンプリング周波数
  * @note ToDo 実行前後でミュート処理関数を呼び出される
  */
 void i2s_mclk_change_clock(uint32_t audio_clock);
 
 /**
  * @brief i2sのバッファにUSBから送られてきたuint8_tデータを格納する
  * 
  * @param in 格納するデータ
  * @param sample 格納するバイト数
  * @param resolution サンプルのビット深度 (16, 24, 32)
  * @return true 成功
  * @return false 失敗(バッファが一杯)
  */
 bool i2s_enqueue(uint8_t* in, int sample, uint8_t resolution);
 
 /**
  * @brief i2sのバッファからデータを取り出す
  * 
  * @param buff 取り出したデータ
  * @param sample 取り出したバイト数
  * @return true 成功
  * @return false 失敗(バッファが空)
  * @note use_core1がtrueのときに呼び出される
  */
 bool i2s_dequeue(int32_t** buff, int* sample);
 
 /**
  * @brief i2sのバッファの長さを取得する
  * 
  * @return int32_t バッファの長さ
  *
  * FIX: was int8_t; widened to avoid underflow/wrap/truncation and to
  * match the updated implementation.
  */
 int32_t i2s_get_buf_length(void);
 
 /**
  * @brief Get the length of the i2s buffer in microseconds
  * 
  * @return int32_t Buffer length in microseconds
  * @note Length is estimated by last transfer time.
  * @note Data in DMA transfer is not included in buffer length.
  */
 int32_t i2s_get_buf_us(void);

/**
 * @brief Compute clock trim target for drift compensation (USB/I2S).
 *
 * @note Divider application is done inside the DMA IRQ handler at block boundaries.
 */
void i2s_clock_trim(void);
int32_t i2s_clktrim_base_q8(void);
void i2s_clktrim_set_offset_q8(int32_t off_q8);

/**
 * @brief Debug counters to diagnose periodic glitches/dropouts.
 *
 * - underrun: buffer hit 0 while playing (ISR entered mute)
 * - overflow: i2s_enqueue() failed because the queue was full (audio chunk dropped)
 *
 * @note These are best-effort and meant for field diagnosis/tuning.
 */
uint32_t i2s_dbg_get_underrun_count(void);
uint32_t i2s_dbg_get_overflow_count(void);
 
 /**
  * @brief i2sの音量を変更する
  * 
  * @param v 音量
  * @param ch チャンネル 0:L&R 1:L 2:R
  */
 void i2s_volume_change(int16_t v, int8_t ch);
 
 /**
  * @brief i2s再生状態の切り替わりを通知するハンドラを設定する
  * 
  * @param func ExternalFunction形式の関数ポインタ
  */
 void set_playback_handler(ExternalFunction func);
 
 /**
  * @brief core1のmain関数を設定する
  * 
  * @param func Core1MainFunction形式の関数ポインタ
  */
 void set_core1_main_function(Core1MainFunction func);
 
 /**
  * @brief EXDFのLRのビットを交互に並び替える操作の高速化関数
  * 
  * @param x 入力
  */
 __force_inline uint64_t part1by1_32(uint32_t x){
     uint64_t res = x;
     res = (res | (res << 16)) & 0x0000FFFF0000FFFFULL;
     res = (res | (res << 8))  & 0x00FF00FF00FF00FFULL;
     res = (res | (res << 4))  & 0x0F0F0F0F0F0F0F0FULL;
     res = (res | (res << 2))  & 0x3333333333333333ULL;
     res = (res | (res << 1))  & 0x5555555555555555ULL;
     return res;
 }
 
 #endif 