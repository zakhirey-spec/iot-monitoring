"use client";

import Link from "next/link";
import { usePathname } from "next/navigation";
import { useState, useEffect } from "react";
import { subscribeAlarms } from "@/lib/firebase";

export default function Sidebar() {
  const pathname = usePathname();
  const [unreadCount, setUnreadCount] = useState(0);

  useEffect(() => {
    // Listen to latest 20 alarms for unread count
    const unsubscribe = subscribeAlarms(20, (data) => {
      const count = data.filter((alarm) => !alarm.dibaca).length;
      setUnreadCount(count);
    });
    return () => unsubscribe();
  }, []);

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
          background-color: var(--bg-card);
          border-right: 1px solid var(--border-light);
          padding: 2rem 1.5rem;
          display: flex;
          flex-direction: column;
          z-index: 50;
          transition: transform 0.3s ease;
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
          background-color: var(--bg-card);
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
          }
          .sidebar.open {
            transform: translateX(0);
          }
          .mobile-header {
            display: flex;
          }
        }
      `}</style>

      {/* Placeholder Mobile Header (Logic open/close omitted for simplicity, can be added if needed) */}
      <div className="mobile-header glass">
        <div className="brand-text">
          <h1 style={{fontSize: "1.1rem", fontWeight: "bold"}}>IoT Logistik</h1>
        </div>
        <button className="menu-toggle">☰</button>
      </div>

      <aside className="sidebar">
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
                <Link href={item.path} className={`nav-item ${pathname === item.path ? "active" : ""}`}>
                  <span>{item.icon}</span>
                  {item.name}
                </Link>
              </li>
            ))}
          </ul>
        </nav>

        <div className="footer">
          <p>Sistem Monitoring v2.0</p>
          <p style={{marginTop: "0.25rem", opacity: 0.6}}>ESP32 + Firebase</p>
        </div>
      </aside>
    </>
  );
}
