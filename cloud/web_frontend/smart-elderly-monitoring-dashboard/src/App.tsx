import { useCallback, useEffect, useMemo, useRef, useState } from "react";
import { AnimatePresence, motion } from "framer-motion";

type Telemetry = {
  flame_flag0: 0 | 1;
  adc_flame: number;
  smoke_flag: 0 | 1;
  adc_smoke: number;
  rain_flag: 0 | 1;
  updatedAt: number;
};

type Tone = "critical" | "warning" | "calm";

type EventItem = {
  id: string;
  title: string;
  detail: string;
  time: string;
  tone: Tone;
};

type ObsImage = {
  key: string;
  size: number;
  lastModified: string;
  url: string;
};

type AlarmUntil = {
  flameUntil: number;
  smokeUntil: number;
  rainUntil: number;
};

type FieldSpec = {
  key: keyof Telemetry;
  name: string;
  meaning: string;
  tone: Tone;
  isAnalog?: boolean;
  note: string;
};

type SensorGroup = {
  id: string;
  title: string;
  subtitle: string;
  flagKey: keyof Telemetry;
  analogKey?: keyof Telemetry;
  accent: string;
  unit?: string;
};

const maxAdc = 65535;

// 将原始 ADC 值映射为直观的日常浓度/热流密度单位
function getAnalogValueOnly(key: keyof Telemetry, value: number): string | number {
  if (key === "adc_flame") {
    // 火焰检测：adc越小火越大，进行 65535 - value 转换
    const invertedValue = maxAdc - value;
    const ratio = Math.max(0, Math.min(1, invertedValue / maxAdc));
    return (ratio * 15.0).toFixed(2);
  }
  const ratio = Math.max(0, Math.min(1, value / maxAdc));
  if (key === "adc_smoke") {
    // 烟雾浓度 PPM, 正常清洁环境 100-300 PPM, 烟雾告警级别 1000+ PPM
    return Math.round(ratio * 4900 + 100);
  }
  return value;
}

// 获取安全进度条宽度百分比 (0-100)，火焰由于越小火越大需进行反转
function getAnalogPercent(key: keyof Telemetry, value: number): number {
  if (key === "adc_flame") {
    return Math.max(0, Math.min(100, ((maxAdc - value) / maxAdc) * 100));
  }
  return Math.max(0, Math.min(100, (value / maxAdc) * 100));
}

function getUnitText(key: keyof Telemetry): string {
  if (key === "adc_flame") return "kW/m² (热辐射)";
  if (key === "adc_smoke") return "PPM (浓度)";
  return "ADC";
}

const fieldSpecs: FieldSpec[] = [
  {
    key: "flame_flag0",
    name: "flame_flag",
    meaning: "是否检测到火焰",
    tone: "critical",
    note: "0：安全 / 未检测到火焰，1：检测到火焰 / 报警",
  },
  {
    key: "adc_flame",
    name: "adc_flame",
    meaning: "火焰检测值 (热流密度)",
    tone: "critical",
    isAnalog: true,
    note: "转换后单位：kW/m² (安全阈值：通常低于 1.5)",
  },
  {
    key: "smoke_flag",
    name: "smoke_flag",
    meaning: "是否检测到烟雾",
    tone: "warning",
    note: "0：安全 / 未检测到烟雾，1：检测到烟雾 / 报警",
  },
  {
    key: "adc_smoke",
    name: "adc_smoke",
    meaning: "烟雾浓度 (CO/有害气体)",
    tone: "warning",
    isAnalog: true,
    note: "转换后单位：PPM (空气品质，安全阈值：低于 800)",
  },
  {
    key: "rain_flag",
    name: "rain_flag",
    meaning: "是否检测到雨滴 / 漏水",
    tone: "calm",
    note: "0：干燥 / 安全，1：检测到水 / 漏水报警",
  },
];

const sensorGroups: SensorGroup[] = [
  {
    id: "flame",
    title: "火焰监测",
    subtitle: "对应 flame_flag 与 adc_flame",
    flagKey: "flame_flag0",
    analogKey: "adc_flame",
    accent: "from-orange-400 via-amber-300 to-rose-400",
    unit: "kW/m²",
  },
  {
    id: "smoke",
    title: "烟雾监测",
    subtitle: "对应 smoke_flag 与 adc_smoke",
    flagKey: "smoke_flag",
    analogKey: "adc_smoke",
    accent: "from-sky-400 via-cyan-300 to-indigo-400",
    unit: "PPM",
  },
  {
    id: "rain",
    title: "漏水监测",
    subtitle: "对应 rain_flag",
    flagKey: "rain_flag",
    accent: "from-cyan-300 via-blue-400 to-sky-500",
  },
];

const initialTelemetry = (): Telemetry => ({
  flame_flag0: 0,
  adc_flame: 16384,
  smoke_flag: 0,
  adc_smoke: 14144,
  rain_flag: 0,
  updatedAt: Date.now(),
});

function clamp(value: number, min: number, max: number) {
  return Math.min(max, Math.max(min, value));
}

function randBetween(min: number, max: number) {
  return Math.round(min + Math.random() * (max - min));
}

function drift(value: number, step: number, min: number, max: number) {
  return clamp(value + randBetween(-step, step), min, max);
}

function formatClock(timestamp: number) {
  return new Intl.DateTimeFormat("zh-CN", {
    hour: "2-digit",
    minute: "2-digit",
    second: "2-digit",
  }).format(timestamp);
}

function formatPercent(value: number) {
  return `${Math.round((value / maxAdc) * 100)}%`;
}

function flagText(key: keyof Telemetry, value: number) {
  if (key === "rain_flag") {
    return value ? "漏水报警" : "干燥安全";
  }

  return value ? "检测到报警" : "安全正常";
}

function toneText(tone: Tone) {
  if (tone === "critical") return "高危";
  if (tone === "warning") return "关注";
  return "正常";
}

function toneStyles(tone: Tone) {
  if (tone === "critical") {
    return {
      chip: "border-rose-400/30 bg-rose-400/10 text-rose-100",
      dot: "bg-rose-400",
      bar: "bg-gradient-to-r from-rose-400 to-orange-300",
    };
  }

  if (tone === "warning") {
    return {
      chip: "border-amber-300/30 bg-amber-300/10 text-amber-100",
      dot: "bg-amber-300",
      bar: "bg-gradient-to-r from-amber-300 to-yellow-200",
    };
  }

  return {
    chip: "border-emerald-400/20 bg-emerald-400/10 text-emerald-100",
    dot: "bg-emerald-400",
    bar: "bg-gradient-to-r from-emerald-400 to-cyan-300",
  };
}

function extractFromHuaweiPush(payload: Record<string, unknown>): Record<string, unknown> | null {
  // 兼容华为云 IoTDA 规则引擎 HTTP 推送的标准格式：
  // { resource, event, notify_data: { body: { services: [{ properties: {...} }] } } }
  const notifyData = payload.notify_data ?? payload.notifyData;
  if (!notifyData || typeof notifyData !== "object") return null;

  const body = (notifyData as Record<string, unknown>).body;
  if (!body || typeof body !== "object") return null;

  const services = (body as Record<string, unknown>).services;
  if (!Array.isArray(services) || services.length === 0) return null;

  const first = services[0] as Record<string, unknown>;
  const properties = first.properties;
  if (properties && typeof properties === "object") {
    return properties as Record<string, unknown>;
  }

  return first;
}

function normalizeTelemetry(raw: unknown, fallback: Telemetry): Telemetry {
  if (!raw || typeof raw !== "object") return fallback;

  const payload = raw as Record<string, unknown>;

  // 优先识别华为云推送的嵌套结构；否则回退到 data 字段；最后直接把 payload 当数据用
  const data =
    extractFromHuaweiPush(payload) ??
    (payload.data && typeof payload.data === "object"
      ? (payload.data as Record<string, unknown>)
      : payload);

  return {
    flame_flag0: toFlag(data.flame_flag0 ?? data.flameFlag0, fallback.flame_flag0),
    adc_flame: toInt(data.adc_flame ?? data.adcFlame, fallback.adc_flame),
    smoke_flag: toFlag(data.smoke_flag ?? data.smokeFlag, fallback.smoke_flag),
    adc_smoke: toInt(data.adc_smoke ?? data.adcSmoke, fallback.adc_smoke),
    rain_flag: toFlag(data.rain_flag ?? data.rainFlag, fallback.rain_flag),
    updatedAt: Date.now(),
  };
}

function toInt(value: unknown, fallback: number) {
  const parsed = typeof value === "number" ? value : Number(value);
  return Number.isFinite(parsed) ? clamp(Math.round(parsed), 0, maxAdc) : fallback;
}

function toFlag(value: unknown, fallback: 0 | 1) {
  const parsed = typeof value === "number" ? value : Number(value);
  return parsed === 1 ? 1 : parsed === 0 ? 0 : fallback;
}

function createInitialEvents(initial: Telemetry): EventItem[] {
  const now = formatClock(initial.updatedAt);
  return [
    {
      id: "boot",
      title: "系统已上线",
      detail: `当前火焰、烟雾、漏水三路状态均为安全，最近上报时间 ${now}`,
      time: now,
      tone: "calm",
    },
  ];
}

function appendEventBatch(current: EventItem[], batch: EventItem[]) {
  return [...batch, ...current].slice(0, 6);
}

function buildEvents(prev: Telemetry, next: Telemetry): EventItem[] {
  const events: EventItem[] = [];
  const at = formatClock(next.updatedAt);

  if (prev.flame_flag0 !== next.flame_flag0) {
    events.push({
      id: `flame-${next.updatedAt}`,
      title: next.flame_flag0 ? "火焰告警触发" : "火焰恢复安全",
      detail: next.flame_flag0
        ? `flame_flag 从 0 切换到 1，火焰模拟采样值升至 ${next.adc_flame} ADC`
        : `flame_flag 已恢复为 0，火焰模拟采样值回落至 ${next.adc_flame} ADC`,
      time: at,
      tone: next.flame_flag0 ? "critical" : "calm",
    });
  }

  if (prev.smoke_flag !== next.smoke_flag) {
    events.push({
      id: `smoke-${next.updatedAt}`,
      title: next.smoke_flag ? "烟雾告警触发" : "烟雾恢复安全",
      detail: next.smoke_flag
        ? `smoke_flag 从 0 切换到 1，烟雾模拟采样值升至 ${next.adc_smoke} ADC`
        : `smoke_flag 已恢复为 0，烟雾模拟采样值回落至 ${next.adc_smoke} ADC`,
      time: at,
      tone: next.smoke_flag ? "warning" : "calm",
    });
  }

  if (prev.rain_flag !== next.rain_flag) {
    events.push({
      id: `rain-${next.updatedAt}`,
      title: next.rain_flag ? "漏水告警触发" : "漏水恢复干燥",
      detail: next.rain_flag ? "rain_flag 从 0 切换到 1，检测到水滴或漏水风险" : "rain_flag 已恢复为 0，环境重新回到干燥状态",
      time: at,
      tone: next.rain_flag ? "warning" : "calm",
    });
  }

  return events;
}

function advanceDemoTelemetry(prev: Telemetry, alarmUntil: AlarmUntil) {
  const now = Date.now();

  if (now >= alarmUntil.flameUntil && Math.random() < 0.08) {
    alarmUntil.flameUntil = now + randBetween(6500, 12000);
  }

  if (now >= alarmUntil.smokeUntil && Math.random() < 0.07) {
    alarmUntil.smokeUntil = now + randBetween(6500, 12500);
  }

  if (now >= alarmUntil.rainUntil && Math.random() < 0.05) {
    alarmUntil.rainUntil = now + randBetween(5000, 10000);
  }

  const flameAlarm = now < alarmUntil.flameUntil;
  const smokeAlarm = now < alarmUntil.smokeUntil;
  const rainAlarm = now < alarmUntil.rainUntil;
  const flame_flag0: 0 | 1 = flameAlarm ? 1 : 0;
  const smoke_flag: 0 | 1 = smokeAlarm ? 1 : 0;
  const rain_flag: 0 | 1 = rainAlarm ? 1 : 0;

  return {
    flame_flag0,
    adc_flame: flameAlarm ? randBetween(2000, 15000) : drift(prev.adc_flame, 2200, 45000, 61000),
    smoke_flag,
    adc_smoke: smokeAlarm ? randBetween(40000, 57600) : drift(prev.adc_smoke, 1920, 5440, 35200),
    rain_flag,
    updatedAt: now,
  };
}

export default function App() {
  const apiUrl = ((((import.meta as ImportMeta & { env?: { VITE_MONITOR_API_URL?: string } }).env?.VITE_MONITOR_API_URL) ?? "") as string).trim();
  const initialTelemetryRef = useRef(initialTelemetry());
  const telemetryRef = useRef(initialTelemetryRef.current);
  const alarmUntil = useRef<AlarmUntil>({ flameUntil: 0, smokeUntil: 0, rainUntil: 0 });
  const [telemetry, setTelemetry] = useState<Telemetry>(initialTelemetryRef.current);
  const [events, setEvents] = useState<EventItem[]>(() => createInitialEvents(initialTelemetryRef.current));
  const [sourceMode, setSourceMode] = useState(apiUrl ? "云端接入" : "演示模式");
  const [sourceNote, setSourceNote] = useState(apiUrl ? "等待华为云 IoTDA 首次上报" : "本地演示采样正在运行");

  const [images, setImages] = useState<ObsImage[]>([]);
  const [imagesLoading, setImagesLoading] = useState(false);
  const [selectedImage, setSelectedImage] = useState<ObsImage | null>(null);

  const fetchImages = useCallback(async () => {
    setImagesLoading(true);
    try {
      const urls = [
        "http://127.0.0.1:5000/list_images",
        apiUrl ? apiUrl.replace("/api/telemetry", "/api/obs-images") : "",
        "http://localhost:5000/list_images"
      ].filter(Boolean);

      for (const url of urls) {
        try {
          const res = await fetch(url);
          if (res.ok) {
            const data = await res.json();
            if (data.status === "success" && Array.isArray(data.images)) {
              setImages(data.images);
              if (data.images.length > 0) {
                setSelectedImage(prev => {
                  if (prev && data.images.some((img: ObsImage) => img.key === prev.key)) {
                    return data.images.find((img: ObsImage) => img.key === prev.key) || data.images[0];
                  }
                  return data.images[0];
                });
              }
              break;
            }
          }
        } catch (e) {
          // ignore
        }
      }
    } finally {
      setImagesLoading(false);
    }
  }, [apiUrl]);

  useEffect(() => {
    fetchImages();
    const interval = setInterval(fetchImages, 5000);
    return () => clearInterval(interval);
  }, [fetchImages]);

  useEffect(() => {
    telemetryRef.current = telemetry;
  }, [telemetry]);

  useEffect(() => {
    let alive = true;

    const pushNextTelemetry = (next: Telemetry, previous: Telemetry) => {
      telemetryRef.current = next;
      setTelemetry(next);

      const newEvents = buildEvents(previous, next);
      if (newEvents.length > 0) {
        setEvents((current) => appendEventBatch(current, newEvents));
      }
    };

    if (!apiUrl) {
      const tick = () => {
        const previous = telemetryRef.current;
        const next = advanceDemoTelemetry(previous, alarmUntil.current);
        pushNextTelemetry(next, previous);
        setSourceMode("演示模式");
        setSourceNote("本地模拟采样持续刷新，可作为华为云接入前的展示页");
      };

      tick();
      const timer = window.setInterval(tick, 2400);
      return () => window.clearInterval(timer);
    }

    const poll = async () => {
      try {
        const response = await fetch(apiUrl, { cache: "no-store" });
        if (!response.ok) {
          throw new Error(`HTTP ${response.status}`);
        }

        const payload = await response.json();
        const previous = telemetryRef.current;
        const next = normalizeTelemetry(payload, previous);
        pushNextTelemetry(next, previous);
        setSourceMode("云端接入");
        setSourceNote("华为云 IoTDA 数据接口在线");
      } catch {
        if (!alive) return;

        const previous = telemetryRef.current;
        const next = advanceDemoTelemetry(previous, alarmUntil.current);
        pushNextTelemetry(next, previous);
        setSourceMode("云端接入失败");
        setSourceNote("未拿到云端数据，已自动切回演示采样");
      }
    };

    poll();
    const timer = window.setInterval(poll, 3000);

    return () => {
      alive = false;
      window.clearInterval(timer);
    };
  }, [apiUrl]);

  const sensorState = useMemo(
    () =>
      sensorGroups.map((group) => {
        const flagValue = telemetry[group.flagKey] as number;
        const analogValue = group.analogKey ? (telemetry[group.analogKey] as number) : undefined;
        const overallTone: Tone = flagValue ? (group.id === "flame" ? "critical" : "warning") : "calm";

        return {
          ...group,
          flagValue,
          analogValue,
          tone: overallTone,
        };
      }),
    [telemetry]
  );

  const highestPriority = sensorState.some((item) => item.flagValue === 1)
    ? sensorState.find((item) => item.flagValue === 1)?.tone ?? "calm"
    : "calm";

  const overallLabel =
    highestPriority === "critical"
      ? "需要立即关注"
      : highestPriority === "warning"
        ? "建议持续关注"
        : "环境状态正常";

  const sourceTone =
    sourceMode === "云端接入"
      ? "calm"
      : sourceMode === "演示模式"
        ? "calm"
        : "warning";

  return (
    <main className="relative isolate min-h-screen overflow-hidden bg-[#04111d] text-slate-50">
      <div className="pointer-events-none absolute inset-0 bg-[radial-gradient(circle_at_top_left,_rgba(56,189,248,0.22),_transparent_34%),radial-gradient(circle_at_top_right,_rgba(244,114,182,0.18),_transparent_28%),linear-gradient(180deg,_rgba(255,255,255,0.05),_transparent_40%)]" />
      <div className="pointer-events-none absolute inset-0 bg-[linear-gradient(rgba(148,163,184,0.05)_1px,transparent_1px),linear-gradient(90deg,rgba(148,163,184,0.05)_1px,transparent_1px)] bg-[size:56px_56px] opacity-20" />
      <div className="pointer-events-none absolute -left-24 top-24 h-72 w-72 rounded-full bg-cyan-400/10 blur-3xl animate-float-slow" />
      <div className="pointer-events-none absolute right-0 top-1/2 h-96 w-96 rounded-full bg-rose-400/10 blur-3xl animate-float-slow-reverse" />

      <div className="relative mx-auto flex w-full max-w-7xl flex-col px-6 pb-14 pt-6 sm:px-8 lg:px-10">
        <header className="flex flex-col gap-4 border-b border-white/10 pb-5 md:flex-row md:items-end md:justify-between">
          <div>
            <p className="text-xs font-medium uppercase tracking-[0.42em] text-cyan-200/75">
              Huawei Cloud IoTDA Live Care
            </p>
            <div className="mt-3 flex items-center gap-4">
              <div className="flex h-12 w-12 items-center justify-center rounded-2xl border border-white/10 bg-white/5 shadow-[0_0_60px_rgba(34,211,238,0.12)]">
                <svg className="h-6 w-6 text-cyan-200" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth={1.8}>
                  <path d="M4 11.5 12 4l8 7.5" />
                  <path d="M6 10.8V20h12v-9.2" />
                  <path d="M9 20v-5.2h6V20" />
                </svg>
              </div>
              <div>
                <h1 className="text-2xl font-semibold tracking-tight sm:text-3xl">独居老人智慧监护系统</h1>
                <p className="mt-1 text-sm text-slate-300">面向家属的实时环境监护页，专注火焰、烟雾和漏水风险。</p>
              </div>
            </div>
          </div>

          <div className="flex flex-wrap items-center gap-3 text-sm text-slate-300">
            <StatusChip tone={sourceTone}>{sourceMode}</StatusChip>
            <span className="rounded-full border border-white/10 bg-white/5 px-4 py-2">{sourceNote}</span>
            <span className="rounded-full border border-white/10 bg-white/5 px-4 py-2">最近上报 {formatClock(telemetry.updatedAt)}</span>
          </div>
        </header>

        <section className="grid min-h-[calc(100vh-6.75rem)] items-center gap-10 py-10 lg:grid-cols-[1.05fr_0.95fr] lg:py-16">
          <motion.div
            initial={{ opacity: 0, y: 28 }}
            animate={{ opacity: 1, y: 0 }}
            transition={{ duration: 0.8, ease: "easeOut" }}
            className="max-w-2xl"
          >
            <p className="text-sm font-medium uppercase tracking-[0.38em] text-cyan-200/80">
              实时显示火焰、烟雾、漏水告警
            </p>
            <h2 className="mt-6 text-4xl font-semibold leading-tight text-white sm:text-5xl lg:text-6xl">
              让远方的牵挂，有一块可信赖的实时屏幕。
            </h2>
            <p className="mt-6 max-w-xl text-base leading-8 text-slate-300 sm:text-lg">
              页面同步展示华为云平台上报的火焰模拟采样值、烟雾模拟采样值，以及是否漏水等关键状态，
              帮助子女在第一时间判断家中环境是否安全。
            </p>

            <div className="mt-8 flex flex-wrap gap-3">
              <a
                href="#monitoring"
                className="rounded-full bg-cyan-300 px-5 py-3 font-medium text-slate-950 transition hover:bg-cyan-200"
              >
                查看实时监护
              </a>
              <a
                href="#fields"
                className="rounded-full border border-white/15 px-5 py-3 font-medium text-white transition hover:border-cyan-300/60 hover:bg-white/5"
              >
                字段说明
              </a>
            </div>

            <div className="mt-10 grid gap-4 border-t border-white/10 pt-8 sm:grid-cols-3">
              {sensorState.map((item) => (
                <MiniSummary key={item.id} item={item} />
              ))}
            </div>
          </motion.div>

          <motion.div
            initial={{ opacity: 0, scale: 0.96 }}
            animate={{ opacity: 1, scale: 1 }}
            transition={{ duration: 0.9, ease: "easeOut", delay: 0.1 }}
            className="relative mx-auto w-full max-w-[580px]"
          >
            <div className="absolute inset-0 rounded-[2rem] border border-white/10 bg-white/5 shadow-[0_0_0_1px_rgba(255,255,255,0.02),0_0_80px_rgba(8,145,178,0.12)] backdrop-blur-xl" />
            <div className="relative rounded-[2rem] p-5 sm:p-7">
              <div className="mb-4 flex items-center justify-between text-xs text-slate-300">
                <span>家庭环境雷达</span>
                <span>{overallLabel}</span>
              </div>
              <HouseIllustration telemetry={telemetry} />
              <div className="mt-5 border-t border-white/10 pt-4 text-sm text-slate-300">
                当前环境判断：<span className="font-medium text-white">{overallLabel}</span>
                <span className="ml-3 text-slate-400">三路传感器状态实时联动。</span>
              </div>
            </div>
          </motion.div>
        </section>

        {/* 摄像头画面监控 Section */}
        <section id="camera-monitoring" className="border-t border-white/10 py-8">
          <div className="flex flex-col gap-3 sm:flex-row sm:items-end sm:justify-between">
            <div>
              <h3 className="text-2xl font-semibold">摄像头监控 (OBS 实时传图)</h3>
              <p className="mt-2 max-w-2xl text-sm leading-7 text-slate-400">
                实时从华为云 OBS 获取上传的监控图片并展示。您可以在下方选择不同的历史图片进行对比查看。
              </p>
            </div>
            <p className="text-sm text-slate-400 font-mono">自动刷新：每 5 秒</p>
          </div>

          <div className="mt-8 grid gap-6 lg:grid-cols-[1.5fr_1fr]">
            {/* 左侧大图显示窗口 */}
            <div className="relative flex flex-col justify-between overflow-hidden rounded-[1.5rem] border border-white/10 bg-white/5 p-4 min-h-[400px]">
              {selectedImage ? (
                <div className="relative flex flex-col h-full items-center justify-between">
                  <div className="relative w-full flex-grow flex items-center justify-center overflow-hidden rounded-lg bg-black/40 p-2 min-h-[300px]">
                    <img
                      src={selectedImage.url}
                      alt={selectedImage.key}
                      className="max-h-[450px] max-w-full rounded object-contain shadow-lg"
                    />
                  </div>
                  <div className="mt-3 flex w-full items-center justify-between border-t border-white/5 pt-3 text-sm text-slate-300">
                    <div className="flex items-center gap-2">
                      <span className="h-2 w-2 rounded-full bg-emerald-400 animate-pulse" />
                      <span className="font-semibold text-white">{selectedImage.key}</span>
                    </div>
                    <div className="flex gap-4 text-xs text-slate-400 font-mono">
                      <span>大小: {(selectedImage.size / 1024).toFixed(2)} KB</span>
                      <span>上传时间: {selectedImage.lastModified}</span>
                    </div>
                  </div>
                </div>
              ) : (
                <div className="flex flex-grow flex-col items-center justify-center text-slate-400 py-12">
                  <svg className="mb-4 h-12 w-12 text-slate-500 animate-pulse" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                    <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={1.5} d="M4 16l4.586-4.586a2 2 0 012.828 0L16 16m-2-2l1.586-1.586a2 2 0 012.828 0L20 14m-6-6h.01M6 20h12a2 2 0 002-2V6a2 2 0 00-2-2H6a2 2 0 00-2 2v12a2 2 0 002 2z" />
                  </svg>
                  <span>暂无监控图片，请运行传图脚本上传图片</span>
                </div>
              )}
            </div>

            {/* 右侧图片缩略图列表/选择窗口 */}
            <div className="flex flex-col rounded-[1.5rem] border border-white/10 bg-white/5 p-4 max-h-[500px]">
              <div className="border-b border-white/10 pb-3 mb-4 flex items-center justify-between">
                <span className="text-sm font-medium text-slate-300">OBS 桶文件列表 ({images.length})</span>
                {imagesLoading && (
                  <span className="text-xs text-cyan-400 animate-pulse font-mono">获取最新数据中...</span>
                )}
              </div>
              <div className="flex-1 overflow-y-auto space-y-2 pr-1 h-[360px] custom-scrollbar overflow-x-hidden">
                {images.length > 0 ? (
                  images.map((img) => (
                    <button
                      key={img.key}
                      onClick={() => setSelectedImage(img)}
                      className={`flex w-full items-center gap-3 rounded-xl border p-2.5 transition text-left cursor-pointer ${
                        selectedImage?.key === img.key
                          ? "border-cyan-400 bg-cyan-400/10 text-white"
                          : "border-white/5 bg-white/5 hover:border-white/20 hover:bg-white/10 text-slate-300"
                      }`}
                    >
                      <div className="relative h-12 w-12 flex-shrink-0 overflow-hidden rounded bg-black/20">
                        <img
                          src={img.url}
                          alt={img.key}
                          className="h-full w-full object-cover"
                        />
                      </div>
                      <div className="min-w-0 flex-1">
                        <p className="truncate text-sm font-medium">{img.key}</p>
                        <p className="mt-1 text-xs text-slate-500 font-mono">
                          {(img.size / 1024).toFixed(1)} KB
                        </p>
                      </div>
                    </button>
                  ))
                ) : (
                  <div className="flex h-40 flex-col items-center justify-center text-xs text-slate-500">
                    <span>当前 OBS 桶 (l610pic) 内无可用图片</span>
                  </div>
                )}
              </div>
            </div>
          </div>
        </section>

        <section id="monitoring" className="border-t border-white/10 py-8">
          <div className="flex flex-col gap-3 sm:flex-row sm:items-end sm:justify-between">
            <div>
              <h3 className="text-2xl font-semibold">实时监护</h3>
              <p className="mt-2 max-w-2xl text-sm leading-7 text-slate-400">
                这里把三路设备的告警状态和原始 ADC 采样值拆开显示，家属可以一眼看懂哪一路异常。
              </p>
            </div>
            <p className="text-sm text-slate-400">建议刷新节奏：约 2 - 3 秒</p>
          </div>

          <div className="mt-8 divide-y divide-white/10 border-y border-white/10">
            {sensorState.map((item) => (
              <SensorRow key={item.id} item={item} />
            ))}
          </div>
        </section>

        <section id="fields" className="border-t border-white/10 py-8">
          <div className="flex flex-col gap-3 sm:flex-row sm:items-end sm:justify-between">
            <div>
              <h3 className="text-2xl font-semibold">华为云字段说明</h3>
              <p className="mt-2 text-sm leading-7 text-slate-400">把云端上报的 5 个字段和页面上的含义一一对应起来，便于验收和联调。</p>
            </div>
          </div>

          <div className="mt-6 divide-y divide-white/10 border-y border-white/10">
            {fieldSpecs.map((field) => (
              <FieldRow key={field.key} field={field} telemetry={telemetry} />
            ))}
          </div>
        </section>

        <section className="border-t border-white/10 py-8">
          <div className="flex flex-col gap-3 sm:flex-row sm:items-end sm:justify-between">
            <div>
              <h3 className="text-2xl font-semibold">最近变化</h3>
              <p className="mt-2 text-sm leading-7 text-slate-400">当某个 flag 从 0 切换到 1，页面会立刻记录一条变化事件。</p>
            </div>
            <p className="text-sm text-slate-400">最新上报 {formatClock(telemetry.updatedAt)}</p>
          </div>

          <div className="mt-6 space-y-3">
            <AnimatePresence initial={false}>
              {events.map((event) => (
                <motion.article
                  key={event.id}
                  layout
                  initial={{ opacity: 0, y: 16 }}
                  animate={{ opacity: 1, y: 0 }}
                  exit={{ opacity: 0, y: -10 }}
                  transition={{ duration: 0.35, ease: "easeOut" }}
                  className="flex gap-4 border-b border-white/10 pb-4"
                >
                  <div className={`mt-1 h-3 w-3 rounded-full ${toneStyles(event.tone).dot}`} />
                  <div className="min-w-0 flex-1">
                    <div className="flex flex-wrap items-center gap-3">
                      <h4 className="font-medium text-white">{event.title}</h4>
                      <span className={`rounded-full border px-2.5 py-1 text-xs ${toneStyles(event.tone).chip}`}>
                        {toneText(event.tone)}
                      </span>
                      <span className="text-xs text-slate-400">{event.time}</span>
                    </div>
                    <p className="mt-2 text-sm leading-7 text-slate-300">{event.detail}</p>
                  </div>
                </motion.article>
              ))}
            </AnimatePresence>
          </div>

          <div className="mt-6 rounded-2xl border border-amber-300/20 bg-amber-300/5 p-4 text-sm leading-7 text-amber-50/90">
            只要 flame_flag、smoke_flag 或 rain_flag 任意一路为 1，建议先电话确认老人状态，再决定是否上门查看。
          </div>
        </section>
      </div>
    </main>
  );
}

function StatusChip({ tone, children }: { tone: Tone; children: string }) {
  return <span className={`rounded-full border px-4 py-2 text-sm ${toneStyles(tone).chip}`}>{children}</span>;
}

function MiniSummary({ item }: { item: SensorGroup & { flagValue: number; analogValue?: number; tone: Tone } }) {
  const styles = toneStyles(item.tone);
  return (
    <div className="space-y-3 rounded-2xl border border-white/10 bg-white/5 p-4 backdrop-blur-sm">
      <div className="flex items-center justify-between gap-3">
        <div>
          <p className="text-sm font-medium text-white">{item.title}</p>
          <p className="mt-1 text-xs text-slate-400">{item.subtitle}</p>
        </div>
        <span className={`rounded-full border px-2.5 py-1 text-xs ${styles.chip}`}>{flagText(item.flagKey, item.flagValue)}</span>
      </div>
      <div>
        <div className="flex items-end justify-between gap-3">
          <p className="text-2xl font-semibold tabular-nums text-white">
            {item.analogKey ? getAnalogValueOnly(item.analogKey, item.analogValue ?? 0) : item.flagValue}
          </p>
          <p className="text-xs text-slate-400">{item.analogKey ? item.unit : "flag"}</p>
        </div>
        <div className="mt-2 h-1.5 rounded-full bg-white/10">
          <motion.div
            className={`h-full rounded-full ${styles.bar}`}
            animate={{ width: `${item.analogKey ? getAnalogPercent(item.analogKey, item.analogValue ?? 0) : item.flagValue ? 100 : 14}%` }}
            transition={{ duration: 0.6, ease: "easeOut" }}
          />
        </div>
      </div>
    </div>
  );
}

function SensorRow({ item }: { item: SensorGroup & { flagValue: number; analogValue?: number; tone: Tone } }) {
  const styles = toneStyles(item.tone);
  const analogPercent = item.analogKey ? getAnalogPercent(item.analogKey, item.analogValue ?? 0) : item.flagValue ? 100 : 12;

  return (
    <div className="grid gap-4 py-5 lg:grid-cols-[1.2fr_0.8fr_1fr] lg:items-center">
      <div>
        <div className="flex items-center gap-3">
          <div className={`h-2.5 w-2.5 rounded-full ${styles.dot}`} />
          <h4 className="text-lg font-medium text-white">{item.title}</h4>
        </div>
        <p className="mt-2 text-sm leading-7 text-slate-400">{item.subtitle}</p>
      </div>

      <div>
        <p className="text-sm text-slate-400">告警状态</p>
        <div className="mt-2 flex flex-wrap items-center gap-3">
          <span className={`rounded-full border px-3 py-1 text-sm ${styles.chip}`}>{flagText(item.flagKey, item.flagValue)}</span>
          <span className="text-sm text-slate-300">
            {item.flagKey} = <span className="font-medium text-white">{item.flagValue}</span>
          </span>
        </div>
      </div>

      <div>
        <div className="flex items-end justify-between gap-3">
          <div>
            <p className="text-sm text-slate-400">日常监护数值</p>
            <div className="mt-1 flex items-baseline gap-2">
              <span className="text-2xl font-semibold tabular-nums text-white">
                {item.analogKey ? getAnalogValueOnly(item.analogKey, item.analogValue ?? 0) : item.flagValue}
              </span>
              <span className="text-xs text-slate-400">{item.analogKey ? item.unit : "flag"}</span>
            </div>
            {item.analogKey && (
              <p className="mt-1 text-xs text-slate-500">
                (对应原始采样: <span className="font-mono">{item.analogValue}</span> ADC)
              </p>
            )}
          </div>
          <span className="text-sm text-slate-400">{item.analogKey ? `${formatPercent(item.analogValue ?? 0)}` : "0 / 1"}</span>
        </div>
        <div className="mt-3 h-2 rounded-full bg-white/10">
          <motion.div
            className={`h-full rounded-full ${styles.bar}`}
            animate={{ width: `${analogPercent}%` }}
            transition={{ duration: 0.65, ease: "easeOut" }}
          />
        </div>
      </div>
    </div>
  );
}

function FieldRow({ field, telemetry }: { field: FieldSpec; telemetry: Telemetry }) {
  const value = telemetry[field.key] as number;
  const styles = toneStyles(field.tone);

  const displayVal = field.isAnalog ? getAnalogValueOnly(field.key, value) : value;
  const unitText = field.isAnalog ? getUnitText(field.key) : "";

  return (
    <div className="grid gap-4 py-4 lg:grid-cols-[1fr_1.4fr_0.8fr] lg:items-center">
      <div>
        <p className="font-medium text-white">{field.name}</p>
        <p className="mt-1 text-xs uppercase tracking-[0.28em] text-slate-500">{field.key}</p>
      </div>

      <div>
        <p className="text-sm text-slate-300">{field.meaning}</p>
        <p className="mt-1 text-sm leading-7 text-slate-500">{field.note}</p>
      </div>

      <div className="flex items-center justify-between gap-3 lg:justify-end">
        <span className={`rounded-full border px-3 py-1 text-xs ${styles.chip}`}>{field.isAnalog ? "已转换物理值" : "flag 值"}</span>
        <span className="font-semibold tabular-nums text-white">
          {displayVal} <span className="text-xs font-normal text-slate-400">{unitText}</span>
        </span>
      </div>
    </div>
  );
}

function HouseIllustration({ telemetry }: { telemetry: Telemetry }) {
  const flame = telemetry.flame_flag0 === 1;
  const smoke = telemetry.smoke_flag === 1;
  const rain = telemetry.rain_flag === 1;

  return (
    <motion.svg
      viewBox="0 0 560 560"
      className="h-auto w-full"
      initial={{ opacity: 0.65 }}
      animate={{ opacity: 1 }}
      transition={{ duration: 0.7 }}
    >
      <defs>
        <radialGradient id="sceneGlow" cx="50%" cy="50%" r="50%">
          <stop offset="0%" stopColor="rgba(34,211,238,0.28)" />
          <stop offset="100%" stopColor="rgba(34,211,238,0)" />
        </radialGradient>
        <linearGradient id="houseLine" x1="0%" x2="100%" y1="0%" y2="100%">
          <stop offset="0%" stopColor="#e2e8f0" stopOpacity="0.95" />
          <stop offset="100%" stopColor="#94a3b8" stopOpacity="0.45" />
        </linearGradient>
        <linearGradient id="roofGlow" x1="0%" x2="100%" y1="0%" y2="100%">
          <stop offset="0%" stopColor="#22d3ee" stopOpacity="0.92" />
          <stop offset="100%" stopColor="#f472b6" stopOpacity="0.88" />
        </linearGradient>
      </defs>

      <circle cx="280" cy="280" r="195" fill="url(#sceneGlow)" />
      <motion.circle
        cx="280"
        cy="280"
        r="152"
        fill="none"
        stroke="rgba(125,211,252,0.22)"
        strokeWidth="1.5"
        animate={{ scale: [1, 1.04, 1], opacity: [0.55, 0.9, 0.55] }}
        transition={{ duration: 5, repeat: Infinity, ease: "easeInOut" }}
      />
      <motion.circle
        cx="280"
        cy="280"
        r="206"
        fill="none"
        stroke="rgba(244,114,182,0.13)"
        strokeWidth="1.2"
        animate={{ scale: [1, 1.02, 1], opacity: [0.35, 0.7, 0.35] }}
        transition={{ duration: 7, repeat: Infinity, ease: "easeInOut" }}
      />

      <g>
        <motion.path
          d="M160 255 280 145 400 255"
          fill="none"
          stroke="url(#roofGlow)"
          strokeWidth="9"
          strokeLinecap="round"
          strokeLinejoin="round"
          animate={{ strokeOpacity: [0.5, 0.9, 0.5] }}
          transition={{ duration: 4.5, repeat: Infinity, ease: "easeInOut" }}
        />
        <path d="M182 252V382H378V252" fill="rgba(15,23,42,0.45)" stroke="url(#houseLine)" strokeWidth="3" />
        <path d="M160 255 280 145 400 255" fill="none" stroke="url(#houseLine)" strokeWidth="3" strokeLinejoin="round" />
        <path d="M226 382V320H334V382" fill="rgba(30,41,59,0.4)" stroke="url(#houseLine)" strokeWidth="3" />
        <path d="M245 382V345h70v37" fill="none" stroke="url(#houseLine)" strokeWidth="2.5" />
        <path d="M200 285h70v55h-70z" fill={smoke ? "rgba(251,191,36,0.18)" : "rgba(148,163,184,0.12)"} stroke="url(#houseLine)" strokeWidth="2.5" />
        <path d="M290 285h70v55h-70z" fill={flame ? "rgba(251,146,60,0.22)" : "rgba(148,163,184,0.12)"} stroke="url(#houseLine)" strokeWidth="2.5" />
        <path d="M271 325c-10 0-20 8-20 20v37h40v-37c0-12-10-20-20-20Z" fill="rgba(125,211,252,0.14)" stroke="url(#houseLine)" strokeWidth="2.5" />
        <path d="M268 339h24" stroke="rgba(125,211,252,0.55)" strokeWidth="2" strokeLinecap="round" />
        <path d="M252 344h8" stroke="rgba(125,211,252,0.55)" strokeWidth="2" strokeLinecap="round" />
      </g>

      <motion.circle
        cx="223"
        cy="311"
        r={smoke ? 19 : 11}
        fill={smoke ? "rgba(251,191,36,0.45)" : "rgba(148,163,184,0.2)"}
        animate={{ scale: smoke ? [1, 1.12, 1] : [1, 1.02, 1], opacity: smoke ? [0.65, 1, 0.65] : [0.35, 0.55, 0.35] }}
        transition={{ duration: smoke ? 1.8 : 5, repeat: Infinity, ease: "easeInOut" }}
      />
      <motion.circle
        cx="315"
        cy="311"
        r={flame ? 20 : 11}
        fill={flame ? "rgba(251,113,133,0.45)" : "rgba(148,163,184,0.2)"}
        animate={{ scale: flame ? [1, 1.15, 1] : [1, 1.02, 1], opacity: flame ? [0.7, 1, 0.7] : [0.35, 0.55, 0.35] }}
        transition={{ duration: flame ? 1.4 : 5, repeat: Infinity, ease: "easeInOut" }}
      />
      <motion.circle
        cx="280"
        cy="365"
        r={rain ? 17 : 10}
        fill={rain ? "rgba(56,189,248,0.45)" : "rgba(148,163,184,0.18)"}
        animate={{ scale: rain ? [1, 1.18, 1] : [1, 1.02, 1], opacity: rain ? [0.7, 1, 0.7] : [0.32, 0.52, 0.32] }}
        transition={{ duration: rain ? 1.6 : 5, repeat: Infinity, ease: "easeInOut" }}
      />

      <g opacity="0.9">
        <path d="M136 422h288" stroke="rgba(148,163,184,0.25)" strokeWidth="2" strokeLinecap="round" />
        <path d="M170 452h220" stroke="rgba(148,163,184,0.16)" strokeWidth="1.5" strokeLinecap="round" />
        <motion.path
          d="M190 442c24-12 44-10 70 0"
          fill="none"
          stroke="rgba(34,211,238,0.45)"
          strokeWidth="2"
          strokeLinecap="round"
          animate={{ opacity: [0.3, 0.75, 0.3] }}
          transition={{ duration: 4.5, repeat: Infinity, ease: "easeInOut" }}
        />
        <motion.path
          d="M304 442c20-8 41-8 64 0"
          fill="none"
          stroke="rgba(244,114,182,0.42)"
          strokeWidth="2"
          strokeLinecap="round"
          animate={{ opacity: [0.25, 0.7, 0.25] }}
          transition={{ duration: 4.8, repeat: Infinity, ease: "easeInOut" }}
        />
      </g>
    </motion.svg>
  );
}
