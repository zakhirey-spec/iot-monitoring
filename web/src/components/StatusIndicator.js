"use client";
import { useEffect, useState } from "react";
import { subscribeStatus } from "@/lib/firebase";

export default function StatusIndicator() {
  const [status, setStatus] = useState(null);
  const [isOnline, setIsOnline] = useState(false);

  useEffect(() => {
    const unsubscribe = subscribeStatus((data) => {
      setStatus(data);
      if (data && data.uptime) {
        // If data was updated recently (within 2 minutes), consider online
        // For simplicity, we just check if data exists and is updating
        // The real heartbeat is every 60s
        setIsOnline(data.online);
      } else {
        setIsOnline(false);
      }
    });

    // Timeout logic: if no new heartbeat in 90 seconds, mark offline
    const interval = setInterval(() => {
      // In a real app, you'd compare data.timestamp with Date.now()
    }, 10000);

    return () => {
      unsubscribe();
      clearInterval(interval);
    };
  }, []);

  return (
    <div className="status-indicator">
      <style jsx>{`
        .status-indicator {
          display: flex;
          align-items: center;
          gap: 0.75rem;
          padding: 0.5rem 1rem;
          background: var(--bg-card);
          border: 1px solid var(--border-light);
          border-radius: var(--radius-full);
          font-size: 0.85rem;
          font-weight: 500;
        }
        .status-text {
          color: var(--text-secondary);
        }
        .online-text {
          color: var(--success);
          font-weight: 600;
        }
        .offline-text {
          color: var(--danger);
          font-weight: 600;
        }
      `}</style>

      <div className={`pulse-dot ${!isOnline ? "offline" : ""}`}></div>
      <span className="status-text">
        System Status:{" "}
        {isOnline ? (
          <span className="online-text">Online</span>
        ) : (
          <span className="offline-text">Offline</span>
        )}
      </span>
      {isOnline && status?.rssi && (
        <span style={{ color: "var(--text-secondary)", marginLeft: "0.5rem", fontSize: "0.75rem" }}>
          WiFi: {status.rssi} dBm
        </span>
      )}
    </div>
  );
}
