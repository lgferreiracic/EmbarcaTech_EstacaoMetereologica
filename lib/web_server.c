#include "./lib/web_server.h"

const char HTML_DASHBOARD[] =
"<!DOCTYPE html>"
"<html lang=\"pt-BR\">"
"<head>"
"<meta charset=\"UTF-8\">"
"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
"<title>Estação Meteorológica</title>"
"<script src=\"https://cdn.jsdelivr.net/npm/chart.js\"></script>"
"<style>"
"*{margin:0;padding:0;box-sizing:border-box}"
"body{font-family:Arial,sans-serif;background:#f5f5f5;padding:20px}"
".header{background:linear-gradient(135deg,#2c5282,#3182ce);color:white;padding:20px;margin-bottom:20px;box-shadow:0 2px 10px rgba(0,0,0,0.1);border-radius:8px;display:flex;align-items:center;justify-content:space-between}"
".header img{height:45px;filter:drop-shadow(0 2px 4px rgba(0,0,0,0.2))}"
".header h1{font-size:1.8rem;font-weight:300;letter-spacing:1px;margin:0;flex:1;text-align:center}"
".header .info{font-size:1rem;font-weight:500;white-space:nowrap}"
"@media (max-width:768px){.header{flex-direction:column;gap:10px}.header h1{order:1}.header .info{order:2;font-size:0.9rem}}"
".alert{display:none;background:#e53e3e;color:white;padding:15px;text-align:center;margin-bottom:20px}"
".alert.active{display:block}"
".alert button{background:white;color:#e53e3e;border:none;padding:8px 16px;margin-top:10px;cursor:pointer}"
".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(300px,1fr));gap:20px;margin-bottom:20px}"
".card{background:white;padding:20px;border:1px solid #ddd;text-align:center}"
".card h3{color:#333;margin-bottom:10px}"
".value{font-size:2rem;font-weight:bold;color:#2c5282;margin:10px 0}"
".chart{height:150px;margin-top:15px}"
".status{background:white;padding:15px;border:1px solid #ddd;text-align:center}"
".online{color:green}"
".offline{color:red}"
"@media (max-width:768px){"
".grid{grid-template-columns:1fr}"
".value{font-size:1.5rem}"
"}"
"</style>"
"</head>"
"<body>"
"<div class=\"header\">"
"<img src=\"https://i.imgur.com/wVCmCfn.png\" alt=\"Logo\">"
"<h1>Estação Meteorológica</h1>"
"<div class=\"info\">RESTIC 37 - CEPEDI</div>"
"</div>"
"<div class=\"alert\" id=\"alert\">"
"<strong>⚠️ ALERTA!</strong> Valores fora dos limites!"
"<br><button onclick=\"stopAlarm()\">Desativar</button>"
"</div>"
"<div class=\"grid\">"
"<div class=\"card\">"
"<h3>🌡️ Temperatura</h3>"
"<div class=\"value\" id=\"temp\">--°C</div>"
"<div class=\"chart\"><canvas id=\"tempChart\"></canvas></div>"
"</div>"
"<div class=\"card\">"
"<h3>💧 Umidade</h3>"
"<div class=\"value\" id=\"humidity\">--%</div>"
"<div class=\"chart\"><canvas id=\"humidityChart\"></canvas></div>"
"</div>"
"<div class=\"card\">"
"<h3>⛰️ Altitude</h3>"
"<div class=\"value\" id=\"altitude\">--m</div>"
"<div class=\"chart\"><canvas id=\"altitudeChart\"></canvas></div>"
"</div>"
"</div>"
"<div class=\"status\">"
"<div id=\"update\">Última atualização: --:--</div>"
"<div id=\"status\" class=\"online\">● Conectado</div>"
"</div>"
"<script>"
"const chartConfig={type:'line',options:{responsive:true,maintainAspectRatio:false,plugins:{legend:{display:false}},scales:{x:{display:false},y:{beginAtZero:false}},elements:{line:{tension:0.4},point:{radius:2}}}};"
"const tempChart=new Chart(document.getElementById('tempChart'),{...chartConfig,data:{labels:[],datasets:[{data:[],borderColor:'#ff6b6b',backgroundColor:'rgba(255,107,107,0.1)',fill:true}]}});"
"const humidityChart=new Chart(document.getElementById('humidityChart'),{...chartConfig,data:{labels:[],datasets:[{data:[],borderColor:'#4ecdc4',backgroundColor:'rgba(78,205,196,0.1)',fill:true}]}});"
"const altitudeChart=new Chart(document.getElementById('altitudeChart'),{...chartConfig,data:{labels:[],datasets:[{data:[],borderColor:'#45b7d1',backgroundColor:'rgba(69,183,209,0.1)',fill:true}]}});"
"function updateChart(chart,value){"
"const time=new Date().toLocaleTimeString();"
"if(chart.data.labels.length>=10){"
"chart.data.labels.shift();"
"chart.data.datasets[0].data.shift();"
"}"
"chart.data.labels.push(time);"
"chart.data.datasets[0].data.push(value);"
"chart.update('none');"
"}"
"function stopAlarm(){fetch('/stop',{method:'POST'});}"
"function updateData(){"
"fetch('/data')"
".then(res=>res.json())"
".then(data=>{"
"document.getElementById('temp').textContent=data.temp.toFixed(1)+'°C';"
"document.getElementById('humidity').textContent=data.hum.toFixed(1)+'%';"
"document.getElementById('altitude').textContent=data.alt.toFixed(0)+'m';"
"updateChart(tempChart,data.temp);"
"updateChart(humidityChart,data.hum);"
"updateChart(altitudeChart,data.alt);"
"document.getElementById('update').textContent='Última atualização: '+new Date().toLocaleTimeString();"
"document.getElementById('status').className='online';"
"document.getElementById('status').textContent='● Conectado';"
"})"
".catch(()=>{"
"document.getElementById('status').className='offline';"
"document.getElementById('status').textContent='● Desconectado';"
"});"
"}"
"function checkAlarm(){"
"fetch('/alarm')"
".then(res=>res.json())"
".then(data=>{"
"document.getElementById('alert').classList.toggle('active',data.alert===\"1\");"
"})"
".catch(console.error);"
"}"
"updateData();"
"checkAlarm();"
"setInterval(updateData,2000);"
"setInterval(checkAlarm,2000);"
"</script>"
"</body>"
"</html>";

// Função de callback para receber dados TCP
static err_t http_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    if (!p)
    {
        tcp_close(tpcb);
        return ERR_OK;
    }

    char *req = (char *)p->payload;

    const char *header_html =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n"
        "\r\n";

    const char *header_json =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Connection: close\r\n"
        "\r\n";

    char json[64];

    if (p->len >= 6 && strncmp(req, "GET / ", 6) == 0){
        tcp_write(tpcb, header_html, strlen(header_html), TCP_WRITE_FLAG_COPY);
        tcp_write(tpcb, HTML_DASHBOARD, strlen(HTML_DASHBOARD), TCP_WRITE_FLAG_COPY);
    }else if(handle_http_request(req, p->len, json, sizeof(json))) {
        tcp_write(tpcb, header_json, strlen(header_json), TCP_WRITE_FLAG_COPY);
        tcp_write(tpcb, json, strlen(json), TCP_WRITE_FLAG_COPY);
    } else {
        const char *not_found =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n"
            "\r\n"
            "Recurso não encontrado.";
        tcp_write(tpcb, not_found, strlen(not_found), TCP_WRITE_FLAG_COPY);
    }
    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);
    tcp_close(tpcb); // Fecha a conexão após envio da resposta
    return ERR_OK;
}

// Função de callback ao aceitar conexões TCP
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err){
    tcp_recv(newpcb, http_recv);
    return ERR_OK;
}

// Função para inicializar o servidor TCP
void server_init(void) {
    //Inicializa a arquitetura do cyw43
    if (cyw43_arch_init()) {
        panic("Failed to initialize CYW43");
    }

    // GPIO do CI CYW43 em nível baixo
    cyw43_arch_gpio_put(LED_PIN, 1);

    // Ativa o Wi-Fi no modo Station, de modo a que possam ser feitas ligações a outros pontos de acesso Wi-Fi.
    cyw43_arch_enable_sta_mode();

    // Conectar à rede WiFI 
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        panic("Failed to connect to WiFi");
    }

    // Caso seja a interface de rede padrão - imprimir o IP do dispositivo.
    if (netif_default){
        printf("IP do dispositivo: %s\n", ipaddr_ntoa(&netif_default->ip_addr));
    }

    // Configura o servidor TCP - cria novos PCBs TCP. É o primeiro passo para estabelecer uma conexão TCP.
    struct tcp_pcb *server = tcp_new();
    if (!server){
        panic("Failed to create TCP PCB\n");
    }

    //vincula um PCB (Protocol Control Block) TCP a um endereço IP e porta específicos.
    if (tcp_bind(server, IP_ADDR_ANY, 80) != ERR_OK){
        panic("Failed to bind TCP PCB\n");
    }

    // Coloca um PCB (Protocol Control Block) TCP em modo de escuta, permitindo que ele aceite conexões de entrada.
    server = tcp_listen(server);

    // Define uma função de callback para aceitar conexões TCP de entrada. É um passo importante na configuração de servidores TCP.
    tcp_accept(server, tcp_server_accept);
    printf("Servidor ouvindo na porta 80\n");
}

