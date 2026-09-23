# 独居老人智慧监护系统 —— 开发 / 对接指南

面向项目落地使用的说明，配合 `src/App.tsx` 里的网页一起看。

---

## 一、在 VSCode 里跑起来

1. 安装 **Node.js 18+**：https://nodejs.org/
2. 用 VSCode 打开项目文件夹
3. 打开终端（快捷键 `` Ctrl + ` ``）：

```bash
npm install        # 第一次需要
npm run dev        # 启动开发服务器
```

4. 浏览器打开终端里提示的地址，一般是 `http://localhost:5173/`
5. 代码改动会自动热更新，不用手动刷新

常用命令：

| 命令 | 作用 |
|---|---|
| `npm run dev` | 本地开发 |
| `npm run build` | 打包成 `dist/index.html` |
| `npm run preview` | 本地预览打包产物 |

---

## 二、查看自己用的是哪种数据转发方式（HTTP / AMQP / MQTT）

在华为云 IoTDA 控制台里看：

1. 登录 https://console.huaweicloud.com/
2. 服务列表 → **设备接入 IoTDA** → 选对**区域 / 实例**
3. 左侧菜单 → **规则引擎** → **数据转发规则**
4. 点开任意一条规则，看「动作 / 转发目标」：

| 转发目标 | 协议 |
|---|---|
| 第三方应用服务（HTTP推送） | HTTP / HTTPS |
| AMQP推送消息队列 | AMQP |
| MQTT推送消息队列 | MQTT（仅标准版 / 企业版） |

补充：左侧 → **总览** → **接入信息** 卡片可以看到 AMQP、HTTP、MQTT 各自的接入地址，其中 MQTT 地址是**设备侧**用的，AMQP / HTTP 是**应用侧**用的，别混。

---

## 三、前端不要直接调华为云 API

三条硬约束：

1. 华为云 IoTDA REST API 要 IAM AK/SK 签名，密钥不能暴露在浏览器里；
2. 浏览器 CORS 限制，华为云默认不开放浏览器直调；
3. 实时数据是"推送"模型，HTTP 轮询延迟大。

**正确做法**：`IoTDA → 规则引擎转发 → 你的后端 → 前端`。
`VITE_MONITOR_API_URL` 填的是**你自己后端的接口**，不是华为云接口。

### 推荐三种落地方式

| 方式 | IoTDA 配置 | 你的后端 | 实时性 | 适合 |
|---|---|---|---|---|
| ① HTTP 推送 + 轮询 | 规则引擎 → 第三方应用服务（HTTP推送） | Node/Python 小服务，接 POST 后存内存，再暴露 GET | 秒级 | 课程项目最推荐 |
| ② AMQP + WebSocket | 规则引擎 → AMQP 推送 | 消费 AMQP 后转 WebSocket 推给前端 | 亚秒级 | 正式项目 |
| ③ 查询设备影子 | 无需配置 | 后端定时调 `GET /v5/.../devices/{id}/shadow` | 取决于轮询间隔 | 最省事，不实时 |

---

## 四、华为云推送过来的真实 JSON 长什么样

当你在规则引擎里选了"HTTP 推送"，每次设备上报数据，华为云会 POST 一个这样的 JSON 到你填的 URL：

```json
{
  "resource": "device.message",
  "event": "report",
  "event_time": "20260105T081234Z",
  "notify_data": {
    "header": {
      "app_id": "xxxx",
      "device_id": "xxxx",
      "node_id": "xxxx",
      "product_id": "xxxx",
      "gateway_id": "xxxx"
    },
    "body": {
      "services": [
        {
          "service_id": "ElderMonitor",
          "properties": {
            "flame_flag0": 0,
            "adc_flame": 1024,
            "smoke_flag": 0,
            "adc_smoke": 884,
            "rain_flag": 0
          },
          "event_time": "20260105T081234Z"
        }
      ]
    }
  }
}
```

前端里的 `normalizeTelemetry` 已经做了兼容：
- 直接传扁平字段可以；
- 传上面这种华为云标准嵌套结构也可以（会自动取 `notify_data.body.services[0].properties`）；
- 传 `{ data: {...} }` 这种常见包装格式也行。

所以你的后端想省事，**可以直接把华为云 push 过来的 body 原样透传给前端**，不用自己解析。

---

## 五、把 `VITE_MONITOR_API_URL` 接上

1. 项目根目录新建 `.env` 文件：

```
VITE_MONITOR_API_URL=http://127.0.0.1:3001/api/telemetry
```

2. 重启 `npm run dev`
3. 页面右上方的状态标签会从"演示模式"变成"云端接入"
4. 如果接口挂了，页面会自动切回演示模式并提示"云端接入失败"，不会白屏

---

## 六、一个最简后端示例

见项目里的 `server/demo-backend.js`，它做了两件事：

1. 监听 `POST /api/push`：接收华为云规则引擎推过来的消息，把 5 个字段存到内存里；
2. 监听 `GET /api/telemetry`：给前端轮询用，返回最近一次状态。

启动方法：

```bash
cd server
node demo-backend.js
```

跑起来之后默认监听 `http://127.0.0.1:3001`。

然后在华为云 IoTDA 控制台里：

1. 规则引擎 → 数据转发规则 → 新建规则
2. 触发条件选"设备消息上报"，选择你的产品
3. 动作选"第三方应用服务（HTTP推送）"
4. 推送 URL 填：`http://<你的服务器IP>:3001/api/push`
5. 协议选 HTTP（本地调试）或 HTTPS（生产）
6. 保存并**启用规则**

前端 `.env` 里 `VITE_MONITOR_API_URL` 填 `http://<你的服务器IP>:3001/api/telemetry`，页面就能拿到真实数据了。

---

## 七、字段速查

| 云端字段 | 含义 | 取值 |
|---|---|---|
| `flame_flag0` | 是否检测到火焰 | 0 = 安全，1 = 报警 |
| `adc_flame` | 火焰模拟采样值 | ADC 原始值（0 ~ 4095） |
| `smoke_flag` | 是否检测到烟雾 | 0 = 安全，1 = 报警 |
| `adc_smoke` | 烟雾模拟采样值 | ADC 原始值（0 ~ 4095） |
| `rain_flag` | 是否检测到雨滴 / 漏水 | 0 = 干燥，1 = 报警 |

前端 `src/App.tsx` 里的 `fieldSpecs` 和 `sensorGroups` 就是按这 5 个字段来排的，如果需要调整显示顺序或阈值，直接改那里就行。
