"use client";

export default function StatusCard({ title, value, unit, icon, color = "blue", status = "normal" }) {
  const colorMap = {
    blue: "var(--accent-blue)",
    green: "var(--success)",
    orange: "var(--warning)",
    red: "var(--danger)",
  };

  const activeColor = colorMap[color] || colorMap.blue;

  return (
    <div className="card status-card">
      <style jsx>{`
        .status-card {
          display: flex;
          align-items: flex-start;
          justify-content: space-between;
          border-top: 3px solid ${activeColor};
        }
        .card-content {
          display: flex;
          flex-direction: column;
          gap: 0.5rem;
        }
        .card-title {
          font-size: 0.9rem;
          color: var(--text-secondary);
          font-weight: 500;
          text-transform: uppercase;
          letter-spacing: 0.05em;
        }
        .card-value {
          font-size: 2.5rem;
          font-weight: 700;
          color: var(--text-primary);
          line-height: 1;
        }
        .card-unit {
          font-size: 1rem;
          color: var(--text-secondary);
          font-weight: 500;
          margin-left: 0.25rem;
        }
        .card-icon {
          width: 48px;
          height: 48px;
          border-radius: var(--radius-md);
          background: rgba(255, 255, 255, 0.03);
          display: flex;
          align-items: center;
          justify-content: center;
          font-size: 1.5rem;
          color: ${activeColor};
          box-shadow: inset 0 0 0 1px rgba(255, 255, 255, 0.05);
        }
        .status-badge {
          display: inline-block;
          margin-top: 0.5rem;
          font-size: 0.75rem;
          padding: 0.2rem 0.5rem;
          border-radius: var(--radius-sm);
          font-weight: 600;
        }
        .status-normal { background: rgba(16, 185, 129, 0.15); color: var(--success); }
        .status-warning { background: rgba(245, 158, 11, 0.15); color: var(--warning); }
        .status-danger { background: rgba(239, 68, 68, 0.15); color: var(--danger); }
      `}</style>

      <div className="card-content">
        <h3 className="card-title">{title}</h3>
        <div className="card-value">
          {value}
          <span className="card-unit">{unit}</span>
        </div>
        {status !== "normal" && (
          <div className={`status-badge status-${status}`}>
            {status === "warning" ? "Mendekati Batas" : "Melebihi Batas"}
          </div>
        )}
      </div>
      <div className="card-icon">{icon}</div>
    </div>
  );
}
