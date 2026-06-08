"use client";

import { useAuth } from './AuthProvider';
import { useRouter } from 'next/navigation';
import { useEffect } from 'react';

export default function ProtectedRoute({ children }) {
  const { currentUser, loading } = useAuth();
  const router = useRouter();

  useEffect(() => {
    if (!loading && !currentUser) {
      router.push('/login');
    }
  }, [currentUser, loading, router]);

  if (loading) {
    return (
      <div style={{ display: 'flex', justifyContent: 'center', alignItems: 'center', height: '100vh', width: '100%' }}>
        <p style={{ color: '#fff', fontSize: '1.2rem' }}>Memuat...</p>
      </div>
    );
  }

  // Jika belum login (dan tidak loading), render null sementara useEffect melakukan redirect
  if (!currentUser) {
    return null;
  }

  return <>{children}</>;
}
