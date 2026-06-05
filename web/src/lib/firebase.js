import { initializeApp, getApps } from "firebase/app";
import { getDatabase, ref, onValue, set, push, get, query, orderByChild, limitToLast, update } from "firebase/database";

const requiredEnvVars = [
  'NEXT_PUBLIC_FIREBASE_API_KEY',
  'NEXT_PUBLIC_FIREBASE_AUTH_DOMAIN',
  'NEXT_PUBLIC_FIREBASE_DATABASE_URL',
  'NEXT_PUBLIC_FIREBASE_PROJECT_ID',
  'NEXT_PUBLIC_FIREBASE_STORAGE_BUCKET',
  'NEXT_PUBLIC_FIREBASE_MESSAGING_SENDER_ID',
  'NEXT_PUBLIC_FIREBASE_APP_ID',
];

const missingEnvVars = requiredEnvVars.filter((key) => !process.env[key]);
if (missingEnvVars.length) {
  console.error('Firebase environment variables missing:', missingEnvVars.join(', '));
}

const firebaseConfig = {
  apiKey: process.env.NEXT_PUBLIC_FIREBASE_API_KEY,
  authDomain: process.env.NEXT_PUBLIC_FIREBASE_AUTH_DOMAIN,
  databaseURL: process.env.NEXT_PUBLIC_FIREBASE_DATABASE_URL,
  projectId: process.env.NEXT_PUBLIC_FIREBASE_PROJECT_ID,
  storageBucket: process.env.NEXT_PUBLIC_FIREBASE_STORAGE_BUCKET,
  messagingSenderId: process.env.NEXT_PUBLIC_FIREBASE_MESSAGING_SENDER_ID,
  appId: process.env.NEXT_PUBLIC_FIREBASE_APP_ID,
};

let app;
let database;

try {
  if (!getApps().length) {
    app = initializeApp(firebaseConfig);
  } else {
    app = getApps()[0];
  }
  database = getDatabase(app);
} catch (error) {
  console.error('Firebase initialization failed:', error);
}

const DEFAULT_REALTIME = {
  suhu: 0,
  kelembapan: 0,
  pintu: false,
  kipas: false,
  solenoid: false,
  buzzerMute: false,
  buzzerActive: false,
  manualKipas: false,
  timestamp: 0,
  waktu: '--:--:--',
  alarmSuhu: false,
  alarmKelembapan: false,
  alarmPintu: false,
};

const DEFAULT_STATUS = {
  online: false,
  rssi: 0,
  uptime: 0,
  freeHeap: 0,
  ip: '',
  timestamp: 0,
};

// ==========================================
// 📡 REALTIME DATA FUNCTIONS
// ==========================================

export function subscribeRealtime(callback) {
  if (!database) {
    callback(DEFAULT_REALTIME);
    return () => {};
  }

  const realtimeRef = ref(database, "realtime");
  const unsubscribe = onValue(
    realtimeRef,
    (snapshot) => {
      const data = snapshot.val();
      callback({ ...DEFAULT_REALTIME, ...(data || {}) });
    },
    (error) => {
      console.error('Firebase realtime subscription error:', error);
      callback(DEFAULT_REALTIME);
    }
  );
  return unsubscribe;
}

export function subscribeStatus(callback) {
  if (!database) {
    callback(DEFAULT_STATUS);
    return () => {};
  }

  const statusRef = ref(database, "status");
  const unsubscribe = onValue(
    statusRef,
    (snapshot) => {
      const data = snapshot.val();
      callback({ ...DEFAULT_STATUS, ...(data || {}), timestamp: Date.now() });
    },
    (error) => {
      console.error('Firebase status subscription error:', error);
      callback(DEFAULT_STATUS);
    }
  );
  return unsubscribe;
}

export function subscribeConfig(callback) {
  if (!database) {
    callback(null);
    return () => {};
  }

  const configRef = ref(database, "config");
  const unsubscribe = onValue(
    configRef,
    (snapshot) => {
      callback(snapshot.val());
    },
    (error) => {
      console.error('Firebase config subscription error:', error);
      callback(null);
    }
  );
  return unsubscribe;
}

// ==========================================
// 🎮 KONTROL FUNCTIONS
// ==========================================

export function subscribeKontrol(callback) {
  if (!database) {
    callback({ kipas: 0, solenoid: 0, buzzerMute: 0 });
    return () => {};
  }

  const kontrolRef = ref(database, "kontrol");
  const unsubscribe = onValue(
    kontrolRef,
    (snapshot) => {
      const data = snapshot.val();
      callback({ kipas: 0, solenoid: 0, buzzerMute: 0, ...(data || {}) });
    },
    (error) => {
      console.error('Firebase kontrol subscription error:', error);
      callback({ kipas: 0, solenoid: 0, buzzerMute: 0 });
    }
  );
  return unsubscribe;
}

export async function setKontrolKipas(value) {
  if (!database) {
    throw new Error('Firebase database not initialized');
  }
  const kontrolRef = ref(database, "kontrol/kipas");
  await set(kontrolRef, value);
}

export async function setKontrolSolenoid(value) {
  if (!database) {
    throw new Error('Firebase database not initialized');
  }
  const kontrolRef = ref(database, "kontrol/solenoid");
  await set(kontrolRef, value);
}

export async function setKontrolBuzzerMute(value) {
  if (!database) {
    throw new Error('Firebase database not initialized');
  }
  const kontrolRef = ref(database, "kontrol/buzzerMute");
  await set(kontrolRef, value);
}

// ==========================================
// 📜 LOG/HISTORY FUNCTIONS
// ==========================================

export async function getLogHistory(limit = 100) {
  const logRef = ref(database, "log");
  const logQuery = query(logRef, orderByChild("timestamp"), limitToLast(limit));
  const snapshot = await get(logQuery);
  if (!snapshot.exists()) return [];
  const data = [];
  snapshot.forEach((child) => {
    data.push({ id: child.key, ...child.val() });
  });
  return data.sort((a, b) => a.timestamp - b.timestamp);
}

export function subscribeLogHistory(limit, callback) {
  const logRef = ref(database, "log");
  const logQuery = query(logRef, orderByChild("timestamp"), limitToLast(limit));
  const unsubscribe = onValue(logQuery, (snapshot) => {
    if (!snapshot.exists()) {
      callback([]);
      return;
    }
    const data = [];
    snapshot.forEach((child) => {
      data.push({ id: child.key, ...child.val() });
    });
    callback(data.sort((a, b) => a.timestamp - b.timestamp));
  });
  return unsubscribe;
}

// ==========================================
// 🚨 ALARM FUNCTIONS
// ==========================================

export function subscribeAlarms(limit, callback) {
  const alarmRef = ref(database, "alarm");
  const alarmQuery = query(alarmRef, orderByChild("timestamp"), limitToLast(limit));
  const unsubscribe = onValue(alarmQuery, (snapshot) => {
    if (!snapshot.exists()) {
      callback([]);
      return;
    }
    const data = [];
    snapshot.forEach((child) => {
      data.push({ id: child.key, ...child.val() });
    });
    callback(data.sort((a, b) => b.timestamp - a.timestamp));
  });
  return unsubscribe;
}

export async function markAlarmAsRead(alarmId) {
  const alarmRef = ref(database, `alarm/${alarmId}`);
  await update(alarmRef, { dibaca: true });
}

export async function markAllAlarmsAsRead(alarmIds) {
  const updates = {};
  alarmIds.forEach((id) => {
    updates[`alarm/${id}/dibaca`] = true;
  });
  const rootRef = ref(database);
  await update(rootRef, updates);
}

// ==========================================
// 🔧 UTILITY EXPORTS
// ==========================================
export { database, ref, onValue, set, push, get };