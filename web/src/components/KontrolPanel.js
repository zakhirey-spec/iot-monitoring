"use client";
import { setKontrolKipas, setKontrolSolenoid, setKontrolBuzzerMute } from "@/lib/firebase";
import { useState } from "react";

export default function KontrolPanel({ data }) {
  const [loading, setLoading] = useState("");
  const [lastToggleTime, setLastToggleTime] = useState({});
  const [error, setError] = useState("");

  const safeData = {
    kipas: false,
    solenoid: false,
    buzzerMute: false,
    ...data,
  };

  const handleToggle = async (type, currentValue) => {
    const now = Date.now();
    const lastTime = lastToggleTime[type] || 0;
    const DEBOUNCE_MS = 800;

    if (now - lastTime < DEBOUNCE_MS) {
      return;
    }

    if (loading && loading !== type) {
      return;
    }

    setLoading(type);
    setError("");

    try {
      const newValue = currentValue ? 0 : 1;
      if (type === "kipas") await setKontrolKipas(newValue);
      if (type === "solenoid") await setKontrolSolenoid(newValue);
      if (type === "buzzer") await setKontrolBuzzerMute(newValue);
      setLastToggleTime((prev) => ({ ...prev, [type]: now }));
    } catch (error) {
      console.error("Gagal mengirim kontrol:", error);
      setError("Gagal mengirim perintah ke ESP32");
      setTimeout(() => setError(""), 4000);
    } finally {
      setTimeout(() => setLoading(""), 500);
    }
  };

  return (
    <div className="card glass">
      <style jsx>{`
        .control-list {
          display: flex;
          flex-direction: column;
          gap: 1.25rem;
          margin-top: 1.5rem;
        }
        .control-item {
          display: flex;
          justify-content: space-between;
          align-items: center;
          padding-bottom: 1.25rem;
          border-bottom: 1px solid var(--border-light);
        }
        .control-item:last-child {
          border-bottom: none;
          padding-bottom: 0;
        }
        .control-info {
          display: flex;
          flex-direction: column;
        }
        .control-name {
          font-weight: 600;
          color: var(--text-primary);
        }
        .control-desc {
          font-size: 0.8rem;
          color: var(--text-secondary);
          margin-top: 0.25rem;
        }
        .switch-container {
          display: flex;
          align-items: center;
          gap: 1rem;
        }
        .state-label {
          font-size: 0.8rem;
          font-weight: 600;
          min-width: 60px;
          text-align: right;
        }
        .switch {
          position: relative;
          display: inline-block;
          width: 52px;
          height: 28px;
          flex: 0 0 auto;
          cursor: pointer;
        }
        .switch input {
          opacity: 0;
          width: 100%;
          height: 100%;
          position: absolute;
          top: 0;
          left: 0;
          margin: 0;
          cursor: pointer;
          z-index: 1;
        }
        .switch .slider {
          position: absolute;
          inset: 0;
          background-color: rgba(255,255,255,0.1);
          transition: .4s;
          border-radius: 34px;
          border: 1px solid var(--border-light);
        }
        .switch .slider::before {
          position: absolute;
          content: "";
          height: 20px;
          width: 20px;
          left: 3px;
          bottom: 3px;
          background-color: white;
          transition: .4s;
          border-radius: 50%;
          box-shadow: 0 2px 4px rgba(0,0,0,0.3);
        }
        .switch input:checked + .slider {
          background-color: var(--accent-blue);
          border-color: var(--accent-blue);
          box-shadow: 0 0 10px var(--accent-blue-glow);
        }
        .switch input:checked + .slider::before {
          transform: translateX(24px);
        }
        .switch input:focus + .slider {
          box-shadow: 0 0 10px var(--accent-blue-glow);
        }
        .switch input:disabled + .slider {
          opacity: 0.5;
          cursor: not-allowed;
        }
        .switch:has(input:disabled) {
          cursor: not-allowed;
        }
      `}</style>

      <h3 style={{ fontSize: "1.1rem", marginBottom: "0.5rem" }}>Panel Kontrol</h3>
      <p style={{ fontSize: "0.85rem", color: "var(--text-secondary)" }}>Kendali manual perangkat dari jarak jauh</p>
      {error && (
        <div style={{ color: "var(--danger)", marginBottom: "1rem", fontSize: "0.85rem" }}>
          ⚠️ {error}
        </div>
      )}

      <div className="control-list">
        {/* Kontrol Kipas */}
        <div className="control-item">
          <div className="control-info">
            <span className="control-name">Kipas Sirkulasi</span>
            <span className="control-desc">
              Mode: {data.kipas ? "Manual ON" : "Auto (Standby)"}
            </span>
          </div>
          <div className="switch-container">
            <span
              className="state-label"
              style={{ color: data.kipas ? "var(--accent-blue)" : "var(--text-secondary)" }}
            >
              {data.kipas ? "MANUAL" : "AUTO"}
            </span>
            <label className="switch">
              <input
                type="checkbox"
                checked={!!data.kipas}
                onChange={() => handleToggle("kipas", data.kipas)}
                disabled={loading === "kipas"}
              />
              <span className="slider"></span>
            </label>
          </div>
        </div>

        {/* Kontrol Solenoid */}
        <div className="control-item">
          <div className="control-info">
            <span className="control-name">Kunci Pintu (Solenoid)</span>
            <span className="control-desc">Buka/tutup kunci pintu kontainer</span>
          </div>
          <div className="switch-container">
            <span
              className="state-label"
              style={{ color: data.solenoid ? "var(--danger)" : "var(--success)" }}
            >
              {data.solenoid ? "TERBUKA" : "KUNCI"}
            </span>
            <label className="switch">
              <input
                type="checkbox"
                checked={!!data.solenoid}
                onChange={() => handleToggle("solenoid", data.solenoid)}
                disabled={loading === "solenoid"}
              />
              <span
                className="slider"
                style={{ backgroundColor: data.solenoid ? "var(--danger)" : "" }}
              ></span>
            </label>
          </div>
        </div>

        {/* Kontrol Buzzer Mute */}
        <div className="control-item">
          <div className="control-info">
            <span className="control-name">Mute Buzzer Alarm</span>
            <span className="control-desc">Matikan suara sirine jika ada peringatan</span>
          </div>
          <div className="switch-container">
            <span
              className="state-label"
              style={{ color: data.buzzerMute ? "var(--warning)" : "var(--text-secondary)" }}
            >
              {data.buzzerMute ? "MUTED" : "NORMAL"}
            </span>
            <label className="switch">
              <input
                type="checkbox"
                checked={!!data.buzzerMute}
                onChange={() => handleToggle("buzzer", data.buzzerMute)}
                disabled={loading === "buzzer"}
              />
              <span
                className="slider"
                style={{ backgroundColor: data.buzzerMute ? "var(--warning)" : "" }}
              ></span>
            </label>
          </div>
        </div>
      </div>
    </div>
  );
}