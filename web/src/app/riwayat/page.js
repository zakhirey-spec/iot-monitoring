"use client";
import { useEffect, useState } from "react";
import { subscribeLogHistory } from "@/lib/firebase";
import HistoryChart from "@/components/HistoryChart";

export default function RiwayatPage() {
  const [logs, setLogs] = useState([]);
  const [loading, setLoading] = useState(true);
  const [limit, setLimit] = useState(100);

  useEffect(() => {
    setLoading(true);
    const unsubscribe = subscribeLogHistory(limit, (data) => {
      setLogs(data);
      setLoading(false);
    });
    return () => unsubscribe();
  }, [limit]);

  const formatDate = (ts) => {
    const d = new Date(ts * 1000);
    return d.toLocaleString("id-ID", { 
      year: 'numeric', month: 'short', day: 'numeric', 
      hour: '2-digit', minute: '2-digit', second: '2-digit' 
    });
  };

  return (
    <div className="animate-fade-in">
      <div className="page-header">
        <div>
          <h1 className="page-title">Riwayat Data</h1>
          <p className="subtitle">Log historis sensor dan kondisi kontainer</p>
        </div>
        
        <div style={{display: "flex", gap: "1rem", alignItems: "center"}}>
          <span style={{fontSize: "0.85rem", color: "var(--text-secondary)"}}>Tampilkan:</span>
          <select 
            value={limit} 
            onChange={(e) => setLimit(Number(e.target.value))}
            style={{
              background: "rgba(255,255,255,0.05)", 
              color: "white", 
              border: "1px solid var(--border-light)", 
              padding: "0.5rem", 
              borderRadius: "var(--radius-sm)",
              outline: "none"
            }}
          >
            <option value={50}>50 Data Terakhir</option>
            <option value={100}>100 Data Terakhir</option>
            <option value={500}>500 Data Terakhir</option>
            <option value={1000}>1000 Data Terakhir</option>
          </select>
        </div>
      </div>

      <div className="card glass" style={{ marginBottom: "2rem" }}>
        <h3 style={{ fontSize: "1.1rem", marginBottom: "1.5rem" }}>Grafik Riwayat</h3>
        {loading ? (
          <div style={{ height: "400px", display: "flex", justifyContent: "center", alignItems: "center" }}>
            <div className="spinner"></div>
          </div>
        ) : (
          <HistoryChart data={logs} />
        )}
      </div>

      <div className="card glass">
        <h3 style={{ fontSize: "1.1rem", marginBottom: "1rem" }}>Tabel Data Log</h3>
        
        <div style={{ overflowX: "auto" }}>
          <table className="data-table">
            <thead>
              <tr>
                <th>Waktu</th>
                <th>Suhu (°C)</th>
                <th>Kelembapan (%)</th>
                <th>Status Pintu</th>
                <th>Kipas</th>
              </tr>
            </thead>
            <tbody>
              {logs.slice().reverse().map((log, index) => (
                <tr key={log.id || index}>
                  <td>{formatDate(log.timestamp)}</td>
                  <td>
                    <span style={{ color: log.suhu > 30 ? "var(--danger)" : "inherit" }}>
                      {log.suhu.toFixed(1)}
                    </span>
                  </td>
                  <td>
                    <span style={{ color: log.kelembapan > 80 ? "var(--danger)" : "inherit" }}>
                      {log.kelembapan.toFixed(1)}
                    </span>
                  </td>
                  <td>
                    {log.pintu ? (
                      <span className="badge badge-danger">TERBUKA</span>
                    ) : (
                      <span className="badge badge-success">TERTUTUP</span>
                    )}
                  </td>
                  <td>
                    {log.kipas ? (
                      <span className="badge badge-neutral" style={{color: "var(--accent-blue)"}}>ON</span>
                    ) : (
                      <span className="badge badge-neutral">OFF</span>
                    )}
                  </td>
                </tr>
              ))}
              {logs.length === 0 && !loading && (
                <tr>
                  <td colSpan="5" style={{ textAlign: "center", padding: "2rem", color: "var(--text-secondary)" }}>
                    Tidak ada data log yang tersedia.
                  </td>
                </tr>
              )}
            </tbody>
          </table>
        </div>
      </div>
    </div>
  );
}
