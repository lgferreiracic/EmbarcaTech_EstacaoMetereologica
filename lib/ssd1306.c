#include "ssd1306.h"
#include "font.h"

// Função para inicializar o display OLED SSD1306
void ssd1306_init(ssd1306_t *ssd, uint8_t width, uint8_t height, bool external_vcc, uint8_t address, i2c_inst_t *i2c) {
  ssd->width = width;
  ssd->height = height;
  ssd->pages = height / 8U;
  ssd->address = address;
  ssd->i2c_port = i2c;
  ssd->bufsize = ssd->pages * ssd->width + 1;
  ssd->ram_buffer = calloc(ssd->bufsize, sizeof(uint8_t));
  ssd->ram_buffer[0] = 0x40;
  ssd->port_buffer[0] = 0x80;
}

// Função para configurar o display OLED SSD1306
void ssd1306_config(ssd1306_t *ssd) {
  ssd1306_command(ssd, SET_DISP | 0x00);
  ssd1306_command(ssd, SET_MEM_ADDR);
  ssd1306_command(ssd, 0x01);
  ssd1306_command(ssd, SET_DISP_START_LINE | 0x00);
  ssd1306_command(ssd, SET_SEG_REMAP | 0x01);
  ssd1306_command(ssd, SET_MUX_RATIO);
  ssd1306_command(ssd, HEIGHT - 1);
  ssd1306_command(ssd, SET_COM_OUT_DIR | 0x08);
  ssd1306_command(ssd, SET_DISP_OFFSET);
  ssd1306_command(ssd, 0x00);
  ssd1306_command(ssd, SET_COM_PIN_CFG);
  ssd1306_command(ssd, 0x12);
  ssd1306_command(ssd, SET_DISP_CLK_DIV);
  ssd1306_command(ssd, 0x80);
  ssd1306_command(ssd, SET_PRECHARGE);
  ssd1306_command(ssd, 0xF1);
  ssd1306_command(ssd, SET_VCOM_DESEL);
  ssd1306_command(ssd, 0x30);
  ssd1306_command(ssd, SET_CONTRAST);
  ssd1306_command(ssd, 0xFF);
  ssd1306_command(ssd, SET_ENTIRE_ON);
  ssd1306_command(ssd, SET_NORM_INV);
  ssd1306_command(ssd, SET_CHARGE_PUMP);
  ssd1306_command(ssd, 0x14);
  ssd1306_command(ssd, SET_DISP | 0x01);
}

// Função para enviar um comando para o display OLED SSD1306
void ssd1306_command(ssd1306_t *ssd, uint8_t command) {
  ssd->port_buffer[1] = command;
  i2c_write_blocking(
    ssd->i2c_port,
    ssd->address,
    ssd->port_buffer,
    2,
    false
  );
}

// Função para enviar dados para o display OLED SSD1306
void ssd1306_send_data(ssd1306_t *ssd) {
  ssd1306_command(ssd, SET_COL_ADDR);
  ssd1306_command(ssd, 0);
  ssd1306_command(ssd, ssd->width - 1);
  ssd1306_command(ssd, SET_PAGE_ADDR);
  ssd1306_command(ssd, 0);
  ssd1306_command(ssd, ssd->pages - 1);
  i2c_write_blocking(
    ssd->i2c_port,
    ssd->address,
    ssd->ram_buffer,
    ssd->bufsize,
    false
  );
}

// Função para desenhar um pixel no display OLED SSD1306
void ssd1306_pixel(ssd1306_t *ssd, uint8_t x, uint8_t y, bool value) {
  uint16_t index = (y >> 3) + (x << 3) + 1;
  uint8_t pixel = (y & 0b111);
  if (value)
    ssd->ram_buffer[index] |= (1 << pixel);
  else
    ssd->ram_buffer[index] &= ~(1 << pixel);
}

// Função para preencher o display OLED SSD1306
/*
void ssd1306_fill(ssd1306_t *ssd, bool value) {
  uint8_t byte = value ? 0xFF : 0x00;
  for (uint8_t i = 1; i < ssd->bufsize; ++i)
    ssd->ram_buffer[i] = byte;
}*/

// Função para preencher o display OLED SSD1306
void ssd1306_fill(ssd1306_t *ssd, bool value) {
    // Itera por todas as posições do display
    for (uint8_t y = 0; y < ssd->height; ++y) {
        for (uint8_t x = 0; x < ssd->width; ++x) {
            ssd1306_pixel(ssd, x, y, value);
        }
    }
}

// Função para desenhar um retângulo no display OLED SSD1306
void ssd1306_rect(ssd1306_t *ssd, uint8_t top, uint8_t left, uint8_t width, uint8_t height, bool value, bool fill) {
  for (uint8_t x = left; x < left + width; ++x) {
    ssd1306_pixel(ssd, x, top, value);
    ssd1306_pixel(ssd, x, top + height - 1, value);
  }
  for (uint8_t y = top; y < top + height; ++y) {
    ssd1306_pixel(ssd, left, y, value);
    ssd1306_pixel(ssd, left + width - 1, y, value);
  }

  if (fill) {
    for (uint8_t x = left + 1; x < left + width - 1; ++x) {
      for (uint8_t y = top + 1; y < top + height - 1; ++y) {
        ssd1306_pixel(ssd, x, y, value);
      }
    }
  }
}

// Função para desenhar uma linha no display OLED SSD1306
void ssd1306_line(ssd1306_t *ssd, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool value) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);

    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;

    int err = dx - dy;

    while (true) {
        ssd1306_pixel(ssd, x0, y0, value); // Desenha o pixel atual

        if (x0 == x1 && y0 == y1) break; // Termina quando alcança o ponto final

        int e2 = err * 2;

        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }

        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

// Função para desenhar uma linha horizontal no display OLED SSD1306
void ssd1306_hline(ssd1306_t *ssd, uint8_t x0, uint8_t x1, uint8_t y, bool value) {
  for (uint8_t x = x0; x <= x1; ++x)
    ssd1306_pixel(ssd, x, y, value);
}

// Função para desenhar uma linha vertical no display OLED SSD1306
void ssd1306_vline(ssd1306_t *ssd, uint8_t x, uint8_t y0, uint8_t y1, bool value) {
  for (uint8_t y = y0; y <= y1; ++y)
    ssd1306_pixel(ssd, x, y, value);
}

// Função para desenhar um caractere
void ssd1306_draw_char(ssd1306_t *ssd, char c, uint8_t x, uint8_t y){
  uint16_t index = (c - ' ') * 8;
  char ver=c;
  
  for (uint8_t i = 0; i < 8; ++i)
  {
    uint8_t line = font[index + i];
    for (uint8_t j = 0; j < 8; ++j)
    {
      ssd1306_pixel(ssd, x + i, y + j, line & (1 << j));
    }
  }
}

// Função para desenhar uma string
void ssd1306_draw_string(ssd1306_t *ssd, const char *str, uint8_t x, uint8_t y)
{
  while (*str)
  {
    ssd1306_draw_char(ssd, *str++, x, y);
    x += 8;
    if (x + 8 >= ssd->width)
    {
      x = 0;
      y += 8;
    }
    if (y + 8 >= ssd->height)
    {
      break;
    }
  }
}

// Função para inicializar o display
void display_init(ssd1306_t *ssd){
  i2c_init(I2C_PORT_DISP, 400000);
  gpio_set_function(I2C_SDA_DISP, GPIO_FUNC_I2C);
  gpio_set_function(I2C_SCL_DISP, GPIO_FUNC_I2C);
  gpio_pull_up(I2C_SDA_DISP);
  gpio_pull_up(I2C_SCL_DISP);
  ssd1306_init(ssd, SSD1306_WIDTH, SSD1306_HEIGHT, false, SSD1306_ADDR, I2C_PORT_DISP);
  ssd1306_config(ssd);
  ssd1306_fill(ssd, false);
  ssd1306_send_data(ssd);
}

// Função para atualizar o conteúdo do display com dados do tempo
void display_weather(ssd1306_t *ssd, float bmp280_temperature, float bmp280_altitude, float aht20_temperature, float aht20_humidity, bool web_server_initialized, bool cor) {

  char str_tmp1[6], str_alt[6], str_tmp2[6], str_umi[6];
  sprintf(str_tmp1, "%.1fC", bmp280_temperature);
  sprintf(str_alt, "%.0fm", bmp280_altitude);
  sprintf(str_tmp2, "%.1fC", aht20_temperature);
  sprintf(str_umi, "%.1f%%", aht20_humidity);

  char server_status[20];
  if (web_server_initialized) {
    snprintf(server_status, sizeof(server_status), "Server: ON");
  } else {
    snprintf(server_status, sizeof(server_status), "Server: OFF");
  }

  ssd1306_fill(ssd, !cor);                           // Limpa o display
  ssd1306_rect(ssd, 3, 3, 122, 60, cor, !cor);       // Desenha um retângulo
  ssd1306_line(ssd, 3, 25, 123, 25, cor);            // Desenha uma linha
  ssd1306_line(ssd, 3, 37, 123, 37, cor);            // Desenha uma linha
  ssd1306_draw_string(ssd, "CEPEDI   TIC37", 8, 6);  // Desenha uma string
  ssd1306_draw_string(ssd, server_status, 20, 16);   // Desenha uma string
  ssd1306_draw_string(ssd, "BMP280  AHT10", 10, 28); // Desenha uma string
  ssd1306_line(ssd, 63, 25, 63, 60, cor);            // Desenha uma linha vertical
  ssd1306_draw_string(ssd, str_tmp1, 14, 41);       // Desenha uma string
  ssd1306_draw_string(ssd, str_alt, 14, 52);        // Desenha uma string
  ssd1306_draw_string(ssd, str_tmp2, 73, 41);      // Desenha uma string
  ssd1306_draw_string(ssd, str_umi, 73, 52);       // Desenha uma string
  ssd1306_send_data(ssd);                            // Atualiza o display
}

// Função para limpar o display OLED SSD1306
void ssd1306_clear(ssd1306_t *ssd) {
  ssd1306_fill(ssd, false); // Preenche o buffer com zeros
  ssd1306_send_data(ssd);   // Envia o buffer para o display
}

// Função para exibir a tela sobre
void start_display(ssd1306_t *ssd){
  ssd1306_fill(ssd, false); // Limpa o display
  ssd1306_draw_string(ssd, "CEPEDI   TIC37", 8, 10); // Desenha uma string
  ssd1306_draw_string(ssd, "Estabelecendo", 12, 30); // Desenha uma string
  ssd1306_draw_string(ssd, "Conexao", 33, 48); // Desenha uma string      
  ssd1306_send_data(ssd); // Atualiza o display
}

// Função para exibir um alerta no display OLED SSD1306
void display_alert(ssd1306_t *ssd, float bmp280_temperature, float bmp280_altitude, float aht20_temperature, float aht20_humidity, bool web_server_initialized, bool cor) {
  char str_tmp1[6], str_alt[6], str_tmp2[6], str_umi[6];
  sprintf(str_tmp1, "%.1fC", bmp280_temperature);
  sprintf(str_alt, "%.0fm", bmp280_altitude);
  sprintf(str_tmp2, "%.1fC", aht20_temperature);
  sprintf(str_umi, "%.1f%%", aht20_humidity);

  char server_status[20];
  if (web_server_initialized) {
    snprintf(server_status, sizeof(server_status), "Server: ON");
  } else {
    snprintf(server_status, sizeof(server_status), "Server: OFF");
  }

  ssd1306_fill(ssd, !cor);                           // Limpa o display
  ssd1306_rect(ssd, 3, 3, 122, 60, cor, !cor);       // Desenha um retângulo
  ssd1306_line(ssd, 3, 25, 123, 25, cor);            // Desenha uma linha
  ssd1306_line(ssd, 3, 37, 123, 37, cor);            // Desenha uma linha
  ssd1306_draw_string(ssd, "ALERTA", 40, 8);  // Desenha uma string
  ssd1306_draw_string(ssd, server_status, 20, 16);   // Desenha uma string
  ssd1306_draw_string(ssd, "BMP280  AHT10", 10, 28); // Desenha uma string
  ssd1306_line(ssd, 63, 25, 63, 60, cor);            // Desenha uma linha vertical
  ssd1306_draw_string(ssd, str_tmp1, 14, 41);       // Desenha uma string
  ssd1306_draw_string(ssd, str_alt, 14, 52);        // Desenha uma string
  ssd1306_draw_string(ssd, str_tmp2, 73, 41);      // Desenha uma string
  ssd1306_draw_string(ssd, str_umi, 73, 52);       // Desenha uma string
  ssd1306_send_data(ssd);                            // Atualiza o display

}