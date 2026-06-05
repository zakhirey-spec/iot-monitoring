"use client";
import { useEffect, useState } from "react";
import { subscribeRealtime, subscribeKontrol } from "@/lib/firebase";
import StatusIndicator from "@/components/StatusIndicator";
import StatusCard from "@/components/StatusCard";
import GaugeChart from "@/components/GaugeChart";
import PintuStatus from "@/components/PintuStatus";
import KontrolPanel from "@/components/KontrolPanel";
import AlertPanel from "@/components/AlertPanel";

export default function Dashboard() {
  const [data, setData] = useState({
    suhu: 0,
    kelembapan: 0,
    pintu: false,
    kipas: false,
    buzzerActive: false,
    timestamp: 0,
    waktu: "--:--:--"
  });

  const [kontrol, setKontrol] = useState({
    kipas: 0,
    solenoid: 0,
    buzzerMute: 0,
  });

  const [loading, setLoading] = useState(true);

  useEffect(() => {
    const unsubRealtime = subscribeRealtime((realtimeData) => {
      if (realtimeData) setData(realtimeData);
      setLoading(false);
    });

    const unsubKontrol = subscribeKontrol((kontrolData) => {
      if (kontrolData) setKontrol(kontrolData);
    });

    return () => {
      unsubRealtime();
      unsubKontrol();
    };
  }, []);

  if (loading) {
    return (
      <div style={{ display: "flex", justifyContent: "center", alignItems: "center", height: "100%" }}>
        <div className="spinner"></div>
      </div>
    );
  }

  const suhuStatus = data.suhu > 30 ? "danger" : (data.suhu > 28 ? "warning" : "normal");
  const humidStatus = data.kelembapan > 80 ? "danger" : (data.kelembapan > 75 ? "warning" : "normal");

  return (
    <div className="animate-fade-in">
      <div className="page-header">
        <div>
          <h1 className="page-title">Monitoring Dashboard</h1>
          <p className="subtitle">Update terakhir: {data.waktu} WIB</p>
        </div>
        <StatusIndicator />
      </div>

      {/* Top Metrics Cards */}
      <div className="grid-cards">
        <StatusCard
          title="Suhu Kontainer"
          value={data.suhu.toFixed(1)}
          unit="°C"
          icon="🌡️"
          color={suhuStatus === "danger" ? "red" : (suhuStatus === "warning" ? "orange" : "blue")}
          status={suhuStatus}
        />
        <StatusCard
          title="Kelembapan"
          value={data.kelembapan.toFixed(1)}
          unit="%"
          icon="💧"
          color={humidStatus === "danger" ? "red" : (humidStatus === "warning" ? "orange" : "blue")}
          status={humidStatus}
        />
        <StatusCard
          title="Sistem Pendingin"
          value={data.kipas ? "AKTIF" : "MATI"}
          unit=""
          icon="🌀"
          color="blue"
        />
      </div>

      <div className="grid-charts">
        {/* Main Content Area */}
        <div className="card glass">
          <h3 style={{ fontSize: "1.1rem", marginBottom: "1.5rem" }}>Visualisasi Real-time</h3>

          <div style={{ display: "grid", gridTemplateColumns: "repeat(auto-fit, minmax(200px, 1fr))", gap: "2rem" }}>
            <GaugeChart
              title="Suhu Udara"
              value={data.suhu}
              min={0} max={50} unit="°C"
              color={suhuStatus === "danger" ? "#EF4444" : (suhuStatus === "warning" ? "#F59E0B" : "#3B82F6")}
            />
            <GaugeChart
              title="Kelembapan"
              value={data.kelembapan}
              min={0} max={100} unit="%"
              color={humidStatus === "danger" ? "#EF4444" : (humidStatus === "warning" ? "#F59E0B" : "#10B981")}
            />
            <PintuStatus isOpen={data.pintu} />
          </div>

          <div style={{ marginTop: "2rem", paddingTop: "1.5rem", borderTop: "1px solid var(--border-light)", display: "flex", gap: "1rem", flexWrap: "wrap" }}>
            <div className={`badge ${data.buzzerActive ? 'badge-danger' : 'badge-neutral'}`}>
              Buzzer: {data.buzzerActive ? "BUNYI" : "SIAP"}
            </div>
            <div className={`badge ${kontrol.buzzerMute ? 'badge-warning' : 'badge-neutral'}`}>
              Mute: {kontrol.buzzerMute ? "AKTIF" : "OFF"}
            </div>
          </div>
        </div>

        {/* Side Panel (Alerts) */}
        <AlertPanel />
      </div>

      {/* Control Panel */}
      <div style={{ maxWidth: "800px" }}>
        <KontrolPanel data={kontrol} />
      </div>
    </div>
  );
}