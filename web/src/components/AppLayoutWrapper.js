"use client";

import { useAuth } from './AuthProvider';
import { usePathname } from 'next/navigation';
import Sidebar from './Sidebar';

export default function AppLayoutWrapper({ children }) {
  const { currentUser, loading } = useAuth();
  const pathname = usePathname();

  const isLoginPage = pathname === '/login';

  return (
    <div className="app-container">
      {!isLoginPage && currentUser && <Sidebar />}
      <main className={`main-content ${!isLoginPage ? 'with-sidebar' : ''}`}>
        {children}
      </main>
    </div>
  );
}
