'use client';

import { useEffect, useState } from 'react';
import styles from './page.module.css';
import ProtectedRoute from '@/components/ProtectedRoute';
import {
  subscribeRealtime,
  subscribeStatus,
  subscribeKontrol,
  setKontrolKipas,
  setKontrolSolenoid,
  setKontrolBuzzerMute,
  subscribeLogHistory,
  subscribeAlarms,
  markAlarmAsRead
} from '../lib/firebase';

import {
  Chart as ChartJS,
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend,
} from 'chart.js';
import { Line } from 'react-chartjs-2';

ChartJS.register(CategoryScale, LinearScale, PointElement, LineElement, Title, Tooltip, Legend);

export default function Home() {
  const [realtime, setRealtime] = useState(null);
  const [status, setStatus] = useState(null);
  const [kontrol, setKontrol] = useState({ kipas: false, solenoid: false, buzzerMute: false });
  const [logs, setLogs] = useState([]);
  const [alarms, setAlarms] = useState([]);

  useEffect(() => {
    const unsubRealtime = subscribeRealtime((data) => setRealtime(data));
    const unsubStatus = subscribeStatus((data) => setStatus(data));
    const unsubKontrol = subscribeKontrol((data) => setKontrol(data));
    const unsubLogs = subscribeLogHistory(50, (data) => setLogs(data));
    const unsubAlarms = subscribeAlarms(20, (data) => setAlarms(data));

    return () => {
      unsubRealtime();
      unsubStatus();
      unsubKontrol();
      unsubLogs();
      unsubAlarms();
    };
  }, []);

  if (!realtime) return <div className={styles.dashboard} style={{color: 'white', textAlign: 'center', marginTop: '5rem'}}>Connecting to Industrial System...</div>;

  const isOnline = status?.online && (Date.now() - status.timestamp < 120000); // 2 mins timeout
  const tempWarning = realtime.suhu > 30;
  const humidWarning = realtime.kelembapan > 80;
  const doorOpen = realtime.pintu;

  // Chart Data Preparation
  const chartData = {
    labels: logs.map(l => l.waktu).slice(-20), // Last 20 logs
    datasets: [
      {
        label: 'Suhu (°C)',
        data: logs.map(l => l.suhu).slice(-20),
        borderColor: '#ef4444',
        backgroundColor: 'rgba(239, 68, 68, 0.2)',
        yAxisID: 'y',
        tension: 0.4,
        fill: true
      },
      {
        label: 'Kelembapan (%)',
        data: logs.map(l => l.kelembapan).slice(-20),
        borderColor: '#3b82f6',
        backgroundColor: 'rgba(59, 130, 246, 0.2)',
        yAxisID: 'y1',
        tension: 0.4,
        fill: true
      }
    ]
  };

  const chartOptions = {
    responsive: true,
    maintainAspectRatio: false,
    interaction: { mode: 'index', intersect: false },
    plugins: {
      legend: { labels: { color: '#94a3b8' } }
    },
    scales: {
      x: { ticks: { color: '#94a3b8' }, grid: { color: 'rgba(255,255,255,0.05)' } },
      y: { type: 'linear', display: true, position: 'left', ticks: { color: '#ef4444' }, grid: { color: 'rgba(255,255,255,0.05)' } },
      y1: { type: 'linear', display: true, position: 'right', ticks: { color: '#3b82f6' }, grid: { drawOnChartArea: false } }
    }
  };

  return (
    <ProtectedRoute>
      <div className={styles.dashboard}>
        {/* Header */}
      <div className={styles.header}>
        <div className={styles.title}>Industrial Monitoring SCADA</div>
        <div className={styles.statusIndicator}>
          <div className={`${styles.dot} ${isOnline ? styles.online : styles.offline}`}></div>
          {isOnline ? `ONLINE - IP: ${status.ip || '-'}` : 'OFFLINE'}
        </div>
      </div>

      {/* Cards Grid */}
      <div className={styles.gridCards}>
        <div className={`${styles.card} ${tempWarning ? styles.danger : styles.normal}`}>
          <div className={styles.cardTitle}>Suhu Ruangan</div>
          <div className={styles.cardValue}>{realtime.suhu?.toFixed(1)} <span className={styles.cardUnit}>°C</span></div>
        </div>
        
        <div className={`${styles.card} ${humidWarning ? styles.warning : styles.normal}`}>
          <div className={styles.cardTitle}>Kelembapan</div>
          <div className={styles.cardValue}>{realtime.kelembapan?.toFixed(1)} <span className={styles.cardUnit}>%</span></div>
        </div>

        <div className={`${styles.card} ${doorOpen ? styles.danger : styles.normal}`}>
          <div className={styles.cardTitle}>Status Pintu</div>
          <div className={styles.cardValue} style={{ color: doorOpen ? '#ef4444' : '#22c55e' }}>
            {doorOpen ? 'TERBUKA' : 'TERKUNCI'}
          </div>
        </div>

        <div className={`${styles.card} ${styles.normal}`}>
          <div className={styles.cardTitle}>Status Hardware</div>
          <div style={{ marginTop: '10px', fontSize: '0.9rem', color: '#94a3b8', lineHeight: '1.8' }}>
            <div>Kipas: <span className={realtime.kipas ? 'badge-success' : 'badge-danger'} style={{padding:'2px 6px', borderRadius:'4px', fontSize:'0.7rem', fontWeight:'bold', marginLeft:'4px'}}>{realtime.kipas ? 'ON' : 'OFF'}</span></div>
            <div>Solenoid: <span className={realtime.solenoid ? 'badge-success' : 'badge-danger'} style={{padding:'2px 6px', borderRadius:'4px', fontSize:'0.7rem', fontWeight:'bold', marginLeft:'4px'}}>{realtime.solenoid ? 'OPEN' : 'LOCKED'}</span></div>
          </div>
        </div>
      </div>

      {/* Control Panel */}
      <div className={styles.controlPanel}>
        <div className={styles.controlHeader}>Control Panel (Manual Override)</div>
        <div className={styles.controlGrid}>
          
          <div className={styles.controlItem}>
            <div className={styles.controlLabel}>Force Kipas ON</div>
            <label className={styles.switch}>
              <input 
                type="checkbox" 
                checked={kontrol.kipas === 1 || kontrol.kipas === true}
                onChange={(e) => setKontrolKipas(e.target.checked ? 1 : 0)} 
              />
              <span className={styles.slider}></span>
            </label>
          </div>

          <div className={styles.controlItem}>
            <div className={styles.controlLabel}>Buka Kunci Solenoid</div>
            <label className={styles.switch}>
              <input 
                type="checkbox" 
                checked={kontrol.solenoid === 1 || kontrol.solenoid === true}
                onChange={(e) => setKontrolSolenoid(e.target.checked ? 1 : 0)} 
              />
              <span className={styles.slider}></span>
            </label>
          </div>

          <div className={styles.controlItem}>
            <div className={styles.controlLabel}>Mute Alarm Buzzer</div>
            <label className={styles.switch}>
              <input 
                type="checkbox" 
                checked={kontrol.buzzerMute === 1 || kontrol.buzzerMute === true}
                onChange={(e) => setKontrolBuzzerMute(e.target.checked ? 1 : 0)} 
              />
              <span className={styles.slider}></span>
            </label>
          </div>

        </div>
      </div>

      {/* Chart Area */}
      <div className={styles.chartsContainer}>
        <div className={styles.controlHeader}>Trend Riwayat Sensor</div>
        <div className={styles.chartWrapper}>
          <Line data={chartData} options={chartOptions} />
        </div>
      </div>

      {/* Tables Area */}
      <div className={styles.tablesContainer}>
        
        {/* Alarms Table */}
        <div className={styles.tableBox}>
          <div className={styles.tableTitle}>Daftar Peringatan (Alarm)</div>
          <table className={styles.dataTable}>
            <thead>
              <tr>
                <th>Waktu</th>
                <th>Pesan</th>
                <th>Status</th>
              </tr>
            </thead>
            <tbody>
              {alarms.map(alarm => {
                const date = new Date(alarm.timestamp * 1000).toLocaleString('id-ID');
                return (
                  <tr key={alarm.id} style={{ opacity: alarm.dibaca ? 0.6 : 1 }}>
                    <td>{date}</td>
                    <td style={{ color: alarm.tipe === 'pintu_terbuka' || alarm.tipe === 'suhu_tinggi' ? '#ef4444' : '#eab308' }}>
                      {alarm.pesan}
                    </td>
                    <td>
                      {!alarm.dibaca ? (
                        <button 
                          onClick={() => markAlarmAsRead(alarm.id)}
                          style={{ padding: '4px 8px', background: '#334155', color: 'white', border: 'none', borderRadius: '4px', fontSize: '0.75rem' }}
                        >
                          Tandai Dibaca
                        </button>
                      ) : (
                        <span style={{ fontSize: '0.8rem', color: '#22c55e' }}>Selesai</span>
                      )}
                    </td>
                  </tr>
                );
              })}
              {alarms.length === 0 && <tr><td colSpan="3" style={{ textAlign: 'center' }}>Tidak ada alarm.</td></tr>}
            </tbody>
          </table>
        </div>

        {/* Logs Table */}
        <div className={styles.tableBox}>
          <div className={styles.tableTitle}>Log Data Signifikan</div>
          <table className={styles.dataTable}>
            <thead>
              <tr>
                <th>Waktu</th>
                <th>Suhu</th>
                <th>Humid</th>
                <th>Pintu</th>
              </tr>
            </thead>
            <tbody>
              {[...logs].reverse().slice(0, 15).map(log => (
                <tr key={log.id}>
                  <td>{log.waktu}</td>
                  <td>{log.suhu?.toFixed(1)}°C</td>
                  <td>{log.kelembapan?.toFixed(1)}%</td>
                  <td>
                    <span className={log.pintu ? 'badge-danger' : 'badge-success'} style={{padding:'4px 8px', borderRadius:'4px', fontSize:'0.7rem', fontWeight:'bold'}}>
                      {log.pintu ? 'BUKA' : 'KUNCI'}
                    </span>
                  </td>
                </tr>
              ))}
              {logs.length === 0 && <tr><td colSpan="4" style={{ textAlign: 'center' }}>Belum ada data log.</td></tr>}
            </tbody>
          </table>
        </div>

      </div>

    </div>
    </ProtectedRoute>
  );
}