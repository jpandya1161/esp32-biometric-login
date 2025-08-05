const socket = io();
let authId = null;
let authToken = null;

socket.on('connect', () => {
    console.log('Connected to server');
    document.getElementById('status').textContent = '✓ Connected';
});

socket.on('disconnect', () => {
    console.log('Disconnected from server');
    document.getElementById('status').textContent = '✗ Disconnected';
});

socket.on('auth_requested', (data) => {
    authId = data.auth_id;
    console.log("Auth requested:", data);
    document.getElementById('message').textContent = data.message;
    document.getElementById('spinner').style.display = 'block';
});

// NEW: When server sends token after recognition
socket.on('auth_token', (data) => {
    console.log("Received auth token:", data.token);
    document.getElementById('spinner').style.display = 'none';
    document.getElementById('status').textContent = '✓ Token received, verifying...';
    authToken = data.token;

    // Store token in sessionStorage
    sessionStorage.setItem('auth_token', authToken);

    // Verify token by sending back to server
    socket.emit('verify_token', { token: authToken });
});

// When token is verified and user ID received
socket.on('auth_verified', (data) => {
    console.log("Token verified, user_id:", data.user_id);
    document.getElementById('status').textContent = `✓ Welcome ${data.user_id}!`;
    document.getElementById('message').textContent = 'Authentication successful! Redirecting...';

    sessionStorage.setItem('user_id', data.user_id);
    sessionStorage.setItem('authenticated', 'true');

    setTimeout(() => {
        window.location.href = '/dashboard?token=' + encodeURIComponent(authToken);
    }, 1000);
});

// If token verification fails
socket.on('auth_failed', (data) => {
    console.log("Token verification failed:", data.message);
    document.getElementById('spinner').style.display = 'none';
    document.getElementById('status').textContent = '⚠ Token verification failed';
    document.getElementById('message').textContent = 'Please try again.';
});

socket.on('auth_timeout', (data) => {
    console.log("Authentication failed:", data.message);
    document.getElementById('spinner').style.display = 'none';
    document.getElementById('status').textContent = '⚠ Face not recognized';
    document.getElementById('message').textContent = 'Please try again.';
});

document.addEventListener('DOMContentLoaded', () => {
    document.getElementById('authBtn').addEventListener('click', () => {
        document.getElementById('message').textContent = 'Please look at the ESP32 camera for authentication';
        document.getElementById('spinner').style.display = 'block';

        socket.emit('request_auth', { message: 'Start biometric recognition' });
    });
});
