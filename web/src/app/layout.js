import "./globals.css";
import { AuthProvider } from "@/components/AuthProvider";
import AppLayoutWrapper from "@/components/AppLayoutWrapper";

export const metadata = {
  title: "IoT Monitoring Kontainer",
  description: "Real-time dashboard untuk sistem monitoring kontainer logistik",
};

export default function RootLayout({ children }) {
  return (
    <html lang="id">
      <head>
        <link rel="preconnect" href="https://fonts.googleapis.com" />
        <link rel="preconnect" href="https://fonts.gstatic.com" crossOrigin="anonymous" />
        <link href="https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700&display=swap" rel="stylesheet" />
      </head>
      <body>
        <AuthProvider>
          <AppLayoutWrapper>
            {children}
          </AppLayoutWrapper>
        </AuthProvider>
      </body>
    </html>
  );
}
