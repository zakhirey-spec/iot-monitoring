"use client";
import { Chart as ChartJS, ArcElement, Tooltip, Legend } from "chart.js";
import { Doughnut } from "react-chartjs-2";

ChartJS.register(ArcElement, Tooltip, Legend);

export default function GaugeChart({ value, min = 0, max = 100, unit = "", title, color = "#3B82F6" }) {
  // Clamp value between min and max
  const clampedValue = Math.min(Math.max(value, min), max);
  const percentage = ((clampedValue - min) / (max - min)) * 100;
  
  const data = {
    datasets: [
      {
        data: [percentage, 100 - percentage],
        backgroundColor: [color, "rgba(255, 255, 255, 0.05)"],
        borderWidth: 0,
        cutout: "80%",
        circumference: 180,
        rotation: 270,
        borderRadius: [4, 0],
      },
    ],
  };

  const options = {
    responsive: true,
    maintainAspectRatio: false,
    plugins: {
      tooltip: { enabled: false },
      legend: { display: false },
    },
    animation: { animateRotate: true, animateScale: false },
  };

  return (
    <div className="gauge-container">
      <style jsx>{`
        .gauge-container {
          position: relative;
          height: 150px;
          display: flex;
          flex-direction: column;
          align-items: center;
          justify-content: flex-end;
          padding-bottom: 1rem;
        }
        .gauge-chart-wrapper {
          position: absolute;
          top: 0;
          left: 0;
          right: 0;
          bottom: 0;
        }
        .gauge-content {
          position: relative;
          text-align: center;
          margin-top: -30px;
        }
        .gauge-value {
          font-size: 2rem;
          font-weight: 700;
          color: var(--text-primary);
          line-height: 1;
        }
        .gauge-unit {
          font-size: 0.9rem;
          color: var(--text-secondary);
        }
        .gauge-title {
          font-size: 0.8rem;
          color: var(--text-secondary);
          text-transform: uppercase;
          letter-spacing: 0.05em;
          margin-top: 0.5rem;
        }
      `}</style>

      <div className="gauge-chart-wrapper">
        <Doughnut data={data} options={options} />
      </div>
      <div className="gauge-content">
        <div className="gauge-value">
          {value.toFixed(1)} <span className="gauge-unit">{unit}</span>
        </div>
        <div className="gauge-title">{title}</div>
      </div>
    </div>
  );
}
