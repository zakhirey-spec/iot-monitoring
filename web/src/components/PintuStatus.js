"use client";

export default function PintuStatus({ isOpen }) {
  return (
    <div className="card">
      <style jsx>{`
        .pintu-container {
          display: flex;
          flex-direction: column;
          align-items: center;
          justify-content: center;
          height: 100%;
          gap: 1rem;
        }
        .door-visual {
          position: relative;
          width: 80px;
          height: 100px;
          border: 4px solid var(--text-secondary);
          border-bottom: none;
          border-radius: 4px 4px 0 0;
          perspective: 1000px;
        }
        .door-panel {
          position: absolute;
          top: 0;
          bottom: 0;
          left: 0;
          width: 100%;
          background: linear-gradient(135deg, var(--accent-blue), #1e3a8a);
          transform-origin: left;
          transition: transform 0.6s cubic-bezier(0.4, 0, 0.2, 1);
          border-radius: 2px 2px 0 0;
          box-shadow: inset -2px 0 5px rgba(0,0,0,0.2);
          display: flex;
          align-items: center;
        }
        .door-handle {
          width: 6px;
          height: 15px;
          background-color: rgba(255,255,255,0.7);
          border-radius: 3px;
          position: absolute;
          right: 8px;
        }
        .door-open .door-panel {
          transform: rotateY(-80deg);
          background: linear-gradient(135deg, var(--danger), #991b1b);
        }
        .status-text {
          font-size: 1.25rem;
          font-weight: 700;
          letter-spacing: 0.05em;
        }
        .text-closed { color: var(--success); }
        .text-open { color: var(--danger); }
        
        .title {
          font-size: 0.9rem;
          color: var(--text-secondary);
          text-transform: uppercase;
          letter-spacing: 0.05em;
          margin-bottom: auto;
          width: 100%;
        }
      `}</style>
      
      <div className="title">Status Pintu Kontainer</div>
      
      <div className="pintu-container">
        <div className={`door-visual ${isOpen ? 'door-open' : ''}`}>
          <div className="door-panel">
            <div className="door-handle"></div>
          </div>
        </div>
        
        <div className={`status-text ${isOpen ? 'text-open' : 'text-closed'}`}>
          {isOpen ? 'TERBUKA' : 'TERTUTUP'}
        </div>
      </div>
    </div>
  );
}
