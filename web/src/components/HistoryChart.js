"use client";
import {
  Chart as ChartJS,
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend,
  Filler
} from "chart.js";
import { Line } from "react-chartjs-2";

ChartJS.register(
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend,
  Filler
);

export default function HistoryChart({ data }) {
  if (!data || data.length === 0) {
    return (
      <div style={{ height: "300px", display: "flex", alignItems: "center", justifyContent: "center", color: "var(--text-secondary)" }}>
        Belum ada data riwayat.
      </div>
    );
  }

  const formatTime = (ts) => {
    const d = new Date(ts * 1000);
    return `${d.getHours().toString().padStart(2,'0')}:${d.getMinutes().toString().padStart(2,'0')}`;
  };

  // Extract labels and datasets
  const labels = data.map((d) => formatTime(d.timestamp));
  const suhuData = data.map((d) => d.suhu);
  const humidData = data.map((d) => d.kelembapan);

  const chartData = {
    labels,
    datasets: [
      {
        label: "Suhu (°C)",
        data: suhuData,
        borderColor: "#3B82F6",
        backgroundColor: "rgba(59, 130, 246, 0.1)",
        borderWidth: 2,
        tension: 0.4,
        fill: true,
        pointRadius: 0,
        pointHitRadius: 10,
        yAxisID: "y",
      },
      {
        label: "Kelembapan (%)",
        data: humidData,
        borderColor: "#10B981",
        backgroundColor: "rgba(16, 185, 129, 0.1)",
        borderWidth: 2,
        tension: 0.4,
        fill: true,
        pointRadius: 0,
        pointHitRadius: 10,
        yAxisID: "y1",
      },
    ],
  };

  const options = {
    responsive: true,
    maintainAspectRatio: false,
    interaction: {
      mode: "index",
      intersect: false,
    },
    plugins: {
      legend: {
        position: "top",
        labels: {
          color: "#94a3b8",
          usePointStyle: true,
          pointStyle: "circle",
        }
      },
      tooltip: {
        backgroundColor: "rgba(10, 22, 40, 0.9)",
        titleColor: "#fff",
        bodyColor: "#e2e8f0",
        borderColor: "rgba(255,255,255,0.1)",
        borderWidth: 1,
        padding: 10,
      }
    },
    scales: {
      x: {
        grid: { color: "rgba(255, 255, 255, 0.05)" },
        ticks: { color: "#94a3b8", maxTicksLimit: 10 }
      },
      y: {
        type: "linear",
        display: true,
        position: "left",
        grid: { color: "rgba(255, 255, 255, 0.05)" },
        ticks: { color: "#94a3b8" },
        title: { display: true, text: "Suhu (°C)", color: "#3B82F6" },
        min: 0,
        max: 50,
      },
      y1: {
        type: "linear",
        display: true,
        position: "right",
        grid: { drawOnChartArea: false },
        ticks: { color: "#94a3b8" },
        title: { display: true, text: "Kelembapan (%)", color: "#10B981" },
        min: 0,
        max: 100,
      },
    },
  };

  return (
    <div style={{ height: "400px", width: "100%" }}>
      <Line data={chartData} options={options} />
    </div>
  );
}
