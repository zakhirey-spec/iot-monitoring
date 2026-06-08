"use client";

import Link from "next/link";
import { usePathname } from "next/navigation";
import { useState, useEffect } from "react";
import { auth, subscribeAlarms } from "@/lib/firebase";
import { signOut } from "firebase/auth";

export default function Sidebar() {
  const pathname = usePathname();
  const [unreadCount, setUnreadCount] = useState(0);
  const [isMobileOpen, setIsMobileOpen] = useState(false);

  useEffect(() => {
    // Listen to latest 20 alarms for unread count
    const unsubscribe = subscribeAlarms(20, (data) => {
      const count = data.filter((alarm) => !alarm.dibaca).length;
      setUnreadCount(count);
    });
    return () => unsubscribe();
  }, []);

  const handleLogout = async () => {
    try {
      await signOut(auth);
    } catch (error) {
      console.error("Logout failed:", error);
    }
  };

  const navItems = [
    { name: "Dashboard", path: "/", icon: "📊" },
    { name: "Riwayat Data", path: "/riwayat", icon: "📈" },
  ];

  return (
    <>
      <style jsx>{`
        .sidebar {
          position: fixed;
          top: 0;
          left: 0;
          bottom: 0;
          width: 280px;
          background: rgba(18, 20, 23, 0.7);
          backdrop-filter: blur(20px);
          -webkit-backdrop-filter: blur(20px);
          border-right: 1px solid var(--border-light);
          padding: 2rem 1.5rem;
          display: flex;
          flex-direction: column;
          z-index: 50;
          transition: transform 0.3s cubic-bezier(0.4, 0, 0.2, 1);
        }

        .brand {
          display: flex;
          align-items: center;
          gap: 0.75rem;
          margin-bottom: 3rem;
          padding: 0 0.5rem;
        }

        .brand-icon {
          font-size: 1.5rem;
          background: linear-gradient(135deg, var(--accent-blue), #60A5FA);
          -webkit-background-clip: text;
          -webkit-text-fill-color: transparent;
        }

        .brand-text h1 {
          font-size: 1.25rem;
          font-weight: 700;
          color: #fff;
          letter-spacing: -0.02em;
        }

        .brand-text p {
          font-size: 0.75rem;
          color: var(--text-secondary);
        }

        .nav-list {
          list-style: none;
          display: flex;
          flex-direction: column;
          gap: 0.5rem;
        }

        .nav-item {
          display: flex;
          align-items: center;
          gap: 1rem;
          padding: 0.75rem 1rem;
          border-radius: var(--radius-sm);
          color: var(--text-secondary);
          font-weight: 500;
          transition: all 0.2s ease;
          position: relative;
        }

        .nav-item:hover {
          color: var(--text-primary);
          background-color: rgba(255, 255, 255, 0.03);
        }

        .nav-item.active {
          color: var(--accent-blue);
          background-color: var(--accent-blue-glow);
        }

        .nav-item.active::before {
          content: '';
          position: absolute;
          left: -1.5rem;
          top: 50%;
          transform: translateY(-50%);
          height: 60%;
          width: 4px;
          background-color: var(--accent-blue);
          border-radius: 0 4px 4px 0;
        }

        .badge-count {
          margin-left: auto;
          background-color: var(--danger);
          color: white;
          font-size: 0.7rem;
          font-weight: bold;
          padding: 0.1rem 0.5rem;
          border-radius: var(--radius-full);
        }

        .footer {
          margin-top: auto;
          padding-top: 2rem;
          border-top: 1px solid var(--border-light);
          font-size: 0.8rem;
          color: var(--text-secondary);
          text-align: center;
        }

        /* Mobile Sidebar */
        .mobile-header {
          display: none;
          position: fixed;
          top: 0;
          left: 0;
          right: 0;
          height: 4rem;
          background: rgba(18, 20, 23, 0.7);
          backdrop-filter: blur(20px);
          -webkit-backdrop-filter: blur(20px);
          border-bottom: 1px solid var(--border-light);
          z-index: 40;
          align-items: center;
          padding: 0 1.5rem;
          justify-content: space-between;
        }

        .menu-toggle {
          background: none;
          border: none;
          color: var(--text-primary);
          font-size: 1.5rem;
          cursor: pointer;
        }

        @media (max-width: 1024px) {
          .sidebar {
            transform: translateX(-100%);
            box-shadow: 20px 0 50px rgba(0,0,0,0.5);
          }
          .sidebar.open {
            transform: translateX(0);
          }
          .mobile-header {
            display: flex;
          }
        }
        
        .overlay {
          display: none;
          position: fixed;
          top: 0; left: 0; right: 0; bottom: 0;
          background: rgba(0,0,0,0.5);
          backdrop-filter: blur(4px);
          z-index: 45;
          transition: opacity 0.3s;
        }
        @media (max-width: 1024px) {
          .overlay.open {
            display: block;
          }
        }
      `}</style>

      {/* Mobile Header */}
      <div className="mobile-header">
        <div className="brand-text">
          <h1 style={{fontSize: "1.1rem", fontWeight: "bold", color: "#fff"}}>IoT Logistik</h1>
        </div>
        <button className="menu-toggle" onClick={() => setIsMobileOpen(!isMobileOpen)}>
          {isMobileOpen ? '✕' : '☰'}
        </button>
      </div>

      <div className={`overlay ${isMobileOpen ? 'open' : ''}`} onClick={() => setIsMobileOpen(false)}></div>

      <aside className={`sidebar ${isMobileOpen ? 'open' : ''}`}>
        <div className="brand">
          <div className="brand-icon">📦</div>
          <div className="brand-text">
            <h1>IoT Logistik</h1>
            <p>Smart Container System</p>
          </div>
        </div>

        <nav>
          <ul className="nav-list">
            {navItems.map((item) => (
              <li key={item.path}>
                <Link 
                  href={item.path} 
                  className={`nav-item ${pathname === item.path ? "active" : ""}`}
                  onClick={() => setIsMobileOpen(false)}
                >
                  <span>{item.icon}</span>
                  {item.name}
                </Link>
              </li>
            ))}
          </ul>
        </nav>

        <div style={{ marginTop: 'auto', padding: '0 0.5rem' }}>
          <button 
            onClick={handleLogout}
            style={{
              width: '100%',
              padding: '0.75rem',
              backgroundColor: 'rgba(248, 81, 73, 0.1)',
              color: '#f85149',
              border: '1px solid rgba(248, 81, 73, 0.2)',
              borderRadius: '8px',
              cursor: 'pointer',
              fontWeight: '500',
              display: 'flex',
              alignItems: 'center',
              justifyContent: 'center',
              gap: '0.5rem',
              transition: 'all 0.2s ease',
              marginBottom: '1rem'
            }}
          >
            <span>🚪</span> Keluar (Logout)
          </button>
        </div>

        <div className="footer" style={{ marginTop: '0' }}>
          <p>Sistem Monitoring v2.0</p>
          <p style={{marginTop: "0.25rem", opacity: 0.6}}>ESP32 + Firebase</p>
        </div>
      </aside>
    </>
  );
}
