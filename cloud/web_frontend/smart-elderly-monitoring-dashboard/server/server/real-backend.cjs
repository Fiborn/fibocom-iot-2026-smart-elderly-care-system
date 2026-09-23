const http = require('http');
const container = require('rhea');

// ==========================================
// 请在这里修改为你的华为云 IoTDA 真实参数
// ==========================================
const HUAWEI_CONFIG = {
    // host: 'e0442c51e1.st1.iotda-app.cn-north-4.myhuaweicloud.com', // 你的AMQP接入域名
    host: 'e0442c51e1.st1.iotda-app.cn-north-4.myhuaweicloud.com',
    port: 5671,
    queue: 'L610message', // 你的AMQP队列名
    accessKey: 'b1pSajd3', // 你的accessKey
    accessCode: 'F2IPyGz35h308Lc3CC4YLe2Ak6P294Gu', // 你的accessCode
    instanceId: 'e0442c51e1' // 👈 新增这一行（提取自你域名的最前端）
};
// ==========================================

// 网页前端的 HTTP 服务配置
const WEB_PORT = 3001;
let latestData = {
    flame_flag0: 0,
    adc_flame: 0,
    smoke_flag: 0,
    adc_smoke: 0,
    rain_flag: 0,
    updatedAt: 0,
};

// 1. 连接华为云 AMQP 的逻辑
// const timestamp = Math.round(new Date());
// const timestamp = Date.now();
// const connection = container.connect({
//     host: HUAWEI_CONFIG.host,
//     port: HUAWEI_CONFIG.port,
//     transport: 'tls',
//     reconnect: true,
//     idle_time_out: 8000,
//     // username: 'accessKey=' + HUAWEI_CONFIG.accessKey + '|timestamp=' + timestamp,
//     username: 'accessKey=' + HUAWEI_CONFIG.accessKey + '|timestamp=' + timestamp + '|instanceId=' + HUAWEI_CONFIG.instanceId,
//     password: HUAWEI_CONFIG.accessCode,
//     sasl_mechanisms: 'PLAIN',
//     rejectUnauthorized: false,
//     hostname: 'default' // 👈 新增这一行
// });

// 1. 连接华为云 AMQP 的逻辑
const timestamp = Date.now(); // 13位毫秒级时间戳

// 打印调试信息，以便你在终端肉眼核对拼接是否正确
console.log('\n================ 调试连接参数 ================');
console.log('1. 接入域名 (host):', HUAWEI_CONFIG.host);
console.log('2. 实例 ID (instanceId):', HUAWEI_CONFIG.instanceId);
console.log('3. 接入键 (accessKey):', HUAWEI_CONFIG.accessKey);
// console.log('4. 拼接后的用户名 (username):', 'accessKey=' + HUAWEI_CONFIG.accessKey + '|timestamp=' + timestamp + '|instanceId=' + HUAWEI_CONFIG.instanceId);
console.log('4. 拼接后的用户名 (username):', 'accessKey=' + HUAWEI_CONFIG.accessKey);
console.log('=============================================\n');

const connection = container.connect({
    host: HUAWEI_CONFIG.host,
    port: HUAWEI_CONFIG.port,
    transport: 'tls',
    reconnect: true,
    idle_time_out: 8000,
    // username: 'accessKey=' + HUAWEI_CONFIG.accessKey + '|timestamp=' + timestamp + '|instanceId=' + HUAWEI_CONFIG.instanceId,
    username: 'accessKey=' + HUAWEI_CONFIG.accessKey,
    password: HUAWEI_CONFIG.accessCode,
    // 采用官方 SDK 的拼写（注意：官方SDK此处确实为双写 n，若仍不行可尝试直接注释掉此行，让 rhea 自动匹配）
    saslMechannisms: 'PLAIN', 
    rejectUnauthorized: false,
    hostname: 'default'
});

const receiver = connection.open_receiver(HUAWEI_CONFIG.queue);

container.on('message', function (context) {
    const msgBody = context.message.body;
    console.log('[AMQP] 收到华为云数据:', msgBody);
    
    try {
        let jsonData;
        if (typeof msgBody === 'string') {
            jsonData = JSON.parse(msgBody);
        } else {
            jsonData = msgBody;
        }

        // 自动解析华为云嵌套结构: notify_data.body.services[0].properties
        if (jsonData.notify_data && jsonData.notify_data.body && jsonData.notify_data.body.services) {
            const props = jsonData.notify_data.body.services[0].properties;
            if (props) {
                latestData.flame_flag0 = props.flame_flag ?? latestData.flame_flag0;
                latestData.adc_flame = props.adc_flame ?? latestData.adc_flame;
                latestData.smoke_flag = props.smoke_flag ?? latestData.smoke_flag;
                latestData.adc_smoke = props.adc_smoke ?? latestData.adc_smoke;
                latestData.rain_flag = props.rain_flag ?? latestData.rain_flag;
                latestData.updatedAt = Date.now();
            }
        } else {
            // 如果不是嵌套结构，直接平铺赋值
            latestData.flame_flag0 = jsonData.flame_flag ?? latestData.flame_flag0;
            latestData.adc_flame = jsonData.adc_flame ?? latestData.adc_flame;
            latestData.smoke_flag = jsonData.smoke_flag ?? latestData.smoke_flag;
            latestData.adc_smoke = jsonData.adc_smoke ?? latestData.adc_smoke;
            latestData.rain_flag = jsonData.rain_flag ?? latestData.rain_flag;
            latestData.updatedAt = Date.now();
        }
    } catch (e) {
        console.error('解析消息失败:', e);
    }
    
    context.delivery.accept();
});

// 2. 提供给网页 of HTTP 接口
const server = http.createServer((req, res) => {
    // CORS 预检
    if (req.method === "OPTIONS") {
        res.writeHead(204, {
            "Access-Control-Allow-Origin": "*",
            "Access-Control-Allow-Methods": "GET, POST, OPTIONS",
            "Access-Control-Allow-Headers": "Content-Type"
        });
        res.end();
        return;
    }

    if (req.method === 'GET' && req.url.startsWith('/api/telemetry')) {
        res.writeHead(200, {
            'Content-Type': 'application/json; charset=utf-8',
            'Access-Control-Allow-Origin': '*',
            'Cache-Control': 'no-store'
        });
        res.end(JSON.stringify(latestData));
    } else if (req.method === "GET" && req.url && req.url.startsWith("/api/obs-images")) {
        const flaskUrl = "http://127.0.0.1:5000/list_images";
        const http2 = require("http");
        const https2 = require("https");
        const urlObj = new URL(flaskUrl);
        const client = urlObj.protocol === "https:" ? https2 : http2;

        const proxyReq = client.get(flaskUrl, (proxyRes) => {
            const chunks = [];
            proxyRes.on("data", (chunk) => chunks.push(chunk));
            proxyRes.on("end", () => {
                try {
                    const body = JSON.parse(Buffer.concat(chunks).toString("utf8"));
                    res.writeHead(200, {
                        'Content-Type': 'application/json; charset=utf-8',
                        'Access-Control-Allow-Origin': '*',
                        'Cache-Control': 'no-store'
                    });
                    res.end(JSON.stringify(body));
                } catch (err) {
                    res.writeHead(502, { 'Content-Type': 'application/json', 'Access-Control-Allow-Origin': '*' });
                    res.end(JSON.stringify({ ok: false, message: "Failed to parse Flask response" }));
                }
            });
        });

        proxyReq.on("error", (err) => {
            res.writeHead(502, { 'Content-Type': 'application/json', 'Access-Control-Allow-Origin': '*' });
            res.end(JSON.stringify({ ok: false, message: "Flask backend unreachable: " + err.message }));
        });

        proxyReq.setTimeout(10000, () => {
            proxyReq.destroy();
            res.writeHead(504, { 'Content-Type': 'application/json', 'Access-Control-Allow-Origin': '*' });
            res.end(JSON.stringify({ ok: false, message: "Flask backend timeout" }));
        });
    } else {
        res.writeHead(404);
        res.end();
    }
});

server.listen(WEB_PORT, () => {
    console.log(`\n========================================`);
    console.log(`✅ 真实数据后端已启动!`);
    console.log(`📡 正在从华为云 AMQP 接收数据...`);
    console.log(`🌐 请将前端接口配置为: http://127.0.0.1:${WEB_PORT}/api/telemetry`);
    console.log(`========================================\n`);
});