/**
 * 独居老人智慧监护系统 —— 最简后端示例
 *
 * 作用：
 *   1) POST /api/push       接收华为云 IoTDA 规则引擎推送的设备消息
 *   2) GET  /api/telemetry  给前端轮询，返回最近一次解析后的状态
 *
 * 启动：
 *   cd server
 *   node demo-backend.js
 *
 * 启动后默认监听 http://127.0.0.1:3001
 *
 * 在华为云 IoTDA 控制台里：
 *   规则引擎 → 数据转发规则 → 新建规则
 *     - 触发条件：设备消息上报（选你的产品）
 *     - 动作：第三方应用服务（HTTP推送）
 *     - 推送 URL：http://<你的服务器IP>:3001/api/push
 *   保存并启用规则即可。
 */

const http = require("http");

const PORT = Number(process.env.PORT || 3001);
const HOST = process.env.HOST || "127.0.0.1";

// 最近一次解析后的状态，前端 GET /api/telemetry 拿到的就是这个结构
let latest = {
  flame_flag0: 0,
  adc_flame: 0,
  smoke_flag: 0,
  adc_smoke: 0,
  rain_flag: 0,
  updatedAt: 0,
};

function toFlag(value, fallback) {
  const n = typeof value === "number" ? value : Number(value);
  return n === 1 ? 1 : n === 0 ? 0 : fallback;
}

function toInt(value, fallback) {
  const n = typeof value === "number" ? value : Number(value);
  return Number.isFinite(n) ? Math.max(0, Math.min(65535, Math.round(n))) : fallback;
}

// 从华为云推送的嵌套结构里把 properties 拿出来
function extractProperties(payload) {
  const notifyData = payload.notify_data || payload.notifyData;
  if (!notifyData || typeof notifyData !== "object") return null;

  const body = notifyData.body;
  if (!body || typeof body !== "object") return null;

  const services = body.services;
  if (!Array.isArray(services) || services.length === 0) return null;

  const first = services[0] || {};
  return first.properties || first;
}

function mergeIntoLatest(data) {
  if (!data || typeof data !== "object") return;

  latest = {
    flame_flag0: toFlag(data.flame_flag0 ?? data.flameFlag0, latest.flame_flag0),
    adc_flame: toInt(data.adc_flame ?? data.adcFlame, latest.adc_flame),
    smoke_flag: toFlag(data.smoke_flag ?? data.smokeFlag, latest.smoke_flag),
    adc_smoke: toInt(data.adc_smoke ?? data.adcSmoke, latest.adc_smoke),
    rain_flag: toFlag(data.rain_flag ?? data.rainFlag, latest.rain_flag),
    updatedAt: Date.now(),
  };
}

function readJsonBody(req) {
  return new Promise((resolve, reject) => {
    const chunks = [];
    req.on("data", (chunk) => chunks.push(chunk));
    req.on("end", () => {
      try {
        const text = Buffer.concat(chunks).toString("utf8") || "{}";
        resolve(JSON.parse(text));
      } catch (err) {
        reject(err);
      }
    });
    req.on("error", reject);
  });
}

function sendJson(res, status, body) {
  res.writeHead(status, {
    "Content-Type": "application/json; charset=utf-8",
    // 让前端网页可以直接跨域请求（调试用，生产环境建议改成具体域名）
    "Access-Control-Allow-Origin": "*",
    "Access-Control-Allow-Methods": "GET, POST, OPTIONS",
    "Access-Control-Allow-Headers": "Content-Type",
    "Cache-Control": "no-store",
  });
  res.end(JSON.stringify(body));
}

const server = http.createServer(async (req, res) => {
  // CORS 预检
  if (req.method === "OPTIONS") {
    sendJson(res, 204, {});
    return;
  }

  // 前端 GET 查询
  if (req.method === "GET" && req.url && req.url.startsWith("/api/telemetry")) {
    sendJson(res, 200, latest);
    return;
  }

  // 华为云推送 POST
  if (req.method === "POST" && req.url && req.url.startsWith("/api/push")) {
    try {
      const payload = await readJsonBody(req);
      // 优先按华为云标准格式解析；否则直接把 payload 当字段用（方便调试）
      const properties = extractProperties(payload) || payload;
      mergeIntoLatest(properties);

      // 华为云要求：收到推送要返回 200，否则平台会重试
      sendJson(res, 200, { ok: true });
    } catch (err) {
      console.error("解析推送失败：", err);
      sendJson(res, 400, { ok: false, message: String(err && err.message || err) });
    }
    return;
  }

  // 代理 OBS 图片列表请求到 Flask 后端
  if (req.method === "GET" && req.url && req.url.startsWith("/api/obs-images")) {
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
          sendJson(res, 200, body);
        } catch (err) {
          sendJson(res, 502, { ok: false, message: "Failed to parse Flask response" });
        }
      });
    });

    proxyReq.on("error", (err) => {
      sendJson(res, 502, { ok: false, message: "Flask backend unreachable: " + err.message });
    });

    proxyReq.setTimeout(10000, () => {
      proxyReq.destroy();
      sendJson(res, 504, { ok: false, message: "Flask backend timeout" });
    });

    return;
  }

  // 健康检查
  if (req.method === "GET" && req.url === "/") {
    sendJson(res, 200, {
      service: "elder-monitor-demo-backend",
      time: new Date().toISOString(),
      latest,
    });
    return;
  }

  sendJson(res, 404, { ok: false, message: "Not Found" });
});

server.listen(PORT, HOST, () => {
  console.log(`[elder-monitor] demo backend running at http://${HOST}:${PORT}`);
  console.log(`  GET  /api/telemetry   -> 前端轮询接口`);
  console.log(`  POST /api/push        -> 华为云规则引擎推送接口`);
  console.log(`在华为云 IoTDA 控制台里把推送 URL 填成：http://<你的IP>:${PORT}/api/push`);
});
