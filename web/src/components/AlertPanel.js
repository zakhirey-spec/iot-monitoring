"use client";
import { useState, useEffect } from "react";
import { subscribeAlarms, markAlarmAsRead, markAllAlarmsAsRead } from "@/lib/firebase";

export default function AlertPanel() {
  const [alarms, setAlarms] = useState([]);
  
  useEffect(() => {
    const unsubscribe = subscribeAlarms(10, (data) => {
      setAlarms(data);
    });
    return () => unsubscribe();
  }, []);

  const unreadCount = alarms.filter(a => !a.dibaca).length;

  const handleMarkAll = async () => {
    const unreadIds = alarms.filter(a => !a.dibaca).map(a => a.id);
    if (unreadIds.length > 0) {
      await markAllAlarmsAsRead(unreadIds);
    }
  };

  const formatTime = (ts) => {
    const d = new Date(ts * 1000);
    return `${d.getHours().toString().padStart(2,'0')}:${d.getMinutes().toString().padStart(2,'0')}`;
  };

  return (
    <div className="card glass" style={{ display: 'flex', flexDirection: 'column', height: '100%', maxHeight: '400px' }}>
      <style jsx>{`
        .alert-header {
          display: flex;
          justify-content: space-between;
          align-items: center;
          margin-bottom: 1rem;
        }
        .alert-list {
          overflow-y: auto;
          flex: 1;
          display: flex;
          flex-direction: column;
          gap: 0.75rem;
          padding-right: 0.5rem;
        }
        /* Scrollbar styling */
        .alert-list::-webkit-scrollbar { width: 4px; }
        .alert-list::-webkit-scrollbar-track { background: transparent; }
        .alert-list::-webkit-scrollbar-thumb { background: rgba(255,255,255,0.1); border-radius: 4px; }
        
        .alert-item {
          padding: 1rem;
          border-radius: var(--radius-sm);
          background: rgba(0,0,0,0.2);
          border-left: 3px solid transparent;
          display: flex;
          gap: 1rem;
          align-items: flex-start;
          transition: background 0.2s ease;
        }
        .alert-item:hover {
          background: rgba(255,255,255,0.03);
        }
        .alert-item.unread {
          background: rgba(255,255,255,0.05);
        }
        .alert-type-suhu_tinggi { border-left-color: var(--danger); }
        .alert-type-kelembapan_tinggi { border-left-color: var(--warning); }
        .alert-type-pintu_terbuka { border-left-color: var(--danger); }
        
        .alert-icon {
          font-size: 1.25rem;
          background: rgba(255,255,255,0.1);
          width: 32px; height: 32px;
          display: flex; align-items: center; justify-content: center;
          border-radius: 50%;
        }
        .alert-content { flex: 1; }
        .alert-message { font-size: 0.85rem; color: var(--text-primary); font-weight: 500; }
        .alert-meta { 
          display: flex; justify-content: space-between; align-items: center;
          font-size: 0.75rem; color: var(--text-secondary); margin-top: 0.5rem; 
        }
        .btn-text {
          background: none; border: none;
          color: var(--accent-blue); font-size: 0.75rem; cursor: pointer;
        }
        .btn-text:hover { text-decoration: underline; }
        
        .empty-state {
          display: flex; flex-direction: column; align-items: center; justify-content: center;
          height: 100%; color: var(--text-secondary); opacity: 0.5; gap: 1rem;
        }
      `}</style>
      
      <div className="alert-header">
        <div style={{display: 'flex', alignItems: 'center', gap: '0.5rem'}}>
          <h3 style={{ fontSize: "1.1rem" }}>Notifikasi Sistem</h3>
          {unreadCount > 0 && <span className="badge badge-danger">{unreadCount} Baru</span>}
        </div>
        {unreadCount > 0 && (
          <button className="btn-text" onClick={handleMarkAll}>Tandai semua dibaca</button>
        )}
      </div>

      {alarms.length === 0 ? (
        <div className="empty-state">
          <div style={{fontSize: '2rem'}}>✓</div>
          <p>Sistem aman. Tidak ada notifikasi.</p>
        </div>
      ) : (
        <div className="alert-list">
          {alarms.map((alarm) => (
            <div key={alarm.id} className={`alert-item alert-type-${alarm.tipe} ${!alarm.dibaca ? 'unread' : ''}`}>
              <div className="alert-icon">
                {alarm.tipe === 'suhu_tinggi' ? '🔥' : alarm.tipe === 'kelembapan_tinggi' ? '💧' : '🚪'}
              </div>
              <div className="alert-content">
                <div className="alert-message">{alarm.pesan}</div>
                <div className="alert-meta">
                  <span>{formatTime(alarm.timestamp)}</span>
                  {!alarm.dibaca && (
                    <button className="btn-text" onClick={() => markAlarmAsRead(alarm.id)}>
                      Tandai dibaca
                    </button>
                  )}
                </div>
              </div>
            </div>
          ))}
        </div>
      )}
    </div>
  );
}
