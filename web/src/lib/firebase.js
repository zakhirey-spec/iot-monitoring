import { initializeApp, getApps } from "firebase/app";
import { getDatabase, ref, onValue, set, push, get, query, orderByChild, limitToLast, update } from "firebase/database";

const firebaseConfig = {
  apiKey: process.env.NEXT_PUBLIC_FIREBASE_API_KEY,
  authDomain: process.env.NEXT_PUBLIC_FIREBASE_AUTH_DOMAIN,
  databaseURL: process.env.NEXT_PUBLIC_FIREBASE_DATABASE_URL,
  projectId: process.env.NEXT_PUBLIC_FIREBASE_PROJECT_ID,
  storageBucket: process.env.NEXT_PUBLIC_FIREBASE_STORAGE_BUCKET,
  messagingSenderId: process.env.NEXT_PUBLIC_FIREBASE_MESSAGING_SENDER_ID,
  appId: process.env.NEXT_PUBLIC_FIREBASE_APP_ID,
};

const app = !getApps().length ? initializeApp(firebaseConfig) : getApps()[0];
const database = getDatabase(app);

// ==========================================
// 📡 REALTIME DATA FUNCTIONS
// ==========================================

export function subscribeRealtime(callback) {
  const realtimeRef = ref(database, "realtime");
  const unsubscribe = onValue(realtimeRef, (snapshot) => {
    const data = snapshot.val();
    callback(data);
  });
  return unsubscribe;
}

export function subscribeStatus(callback) {
  const statusRef = ref(database, "status");
  const unsubscribe = onValue(statusRef, (snapshot) => {
    const data = snapshot.val();
    callback(data);
  });
  return unsubscribe;
}

export function subscribeConfig(callback) {
  const configRef = ref(database, "config");
  const unsubscribe = onValue(configRef, (snapshot) => {
    const data = snapshot.val();
    callback(data);
  });
  return unsubscribe;
}

// ==========================================
// 🎮 KONTROL FUNCTIONS
// ==========================================

export function subscribeKontrol(callback) {
  const kontrolRef = ref(database, "kontrol");
  const unsubscribe = onValue(kontrolRef, (snapshot) => {
    const data = snapshot.val();
    callback(data);
  });
  return unsubscribe;
}

export async function setKontrolKipas(value) {
  const kontrolRef = ref(database, "kontrol/kipas");
  await set(kontrolRef, value);
}

export async function setKontrolSolenoid(value) {
  const kontrolRef = ref(database, "kontrol/solenoid");
  await set(kontrolRef, value);
}

export async function setKontrolBuzzerMute(value) {
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