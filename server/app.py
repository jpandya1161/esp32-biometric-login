import eventlet
eventlet.monkey_patch()

from flask import Flask, render_template, request, jsonify, session, redirect
from flask_socketio import SocketIO, emit
from flask_cors import CORS
from datetime import datetime, timedelta
import secrets
import uuid
import threading
import time
import serial
import jwt

app = Flask(__name__)
app.config['SECRET_KEY'] = secrets.token_urlsafe(32)
app.config['JWT_SECRET_KEY'] = secrets.token_urlsafe(32)
app.config['JWT_ALGORITHM'] = 'HS256'
app.config['JWT_EXPIRATION'] = timedelta(minutes=30)

CORS(app, origins="*")
socketio = SocketIO(app, cors_allowed_origins="*")

auth_requests = {}
valid_tokens = {}

# ---------------- JWT TOKEN UTILS ---------------- #
def generate_token(user_id):
    payload = {
        'user_id': user_id,
        'exp': datetime.utcnow() + app.config['JWT_EXPIRATION']
    }
    token = jwt.encode(payload, app.config['JWT_SECRET_KEY'], algorithm=app.config['JWT_ALGORITHM'])
    valid_tokens[token] = user_id
    return token

def validate_token(token):
    try:
        if token not in valid_tokens:
            return None
        payload = jwt.decode(token, app.config['JWT_SECRET_KEY'], algorithms=[app.config['JWT_ALGORITHM']])
        return payload['user_id']

    except jwt.ExpiredSignatureError:
        valid_tokens.pop(token, None)
        return None
    except jwt.InvalidTokenError:
        return None

def clean_expired_tokens():
    while True:
        time.sleep(300)
        now = datetime.utcnow()
        expired_tokens = []
        for token in list(valid_tokens.keys()):
            try:
                payload = jwt.decode(token, app.config['JWT_SECRET_KEY'], algorithms=[app.config['JWT_ALGORITHM']])
                if payload['exp'] < now.timestamp():
                    expired_tokens.append(token)
            except jwt.ExpiredSignatureError:
                expired_tokens.append(token)
            except jwt.InvalidTokenError:
                expired_tokens.append(token)
        for token in expired_tokens:
            valid_tokens.pop(token, None)

# ---------------- ROUTES ---------------- #
@app.route('/')
def index():
    return render_template('index.html')

@app.route('/auth_result', methods=['POST'])
def handle_auth_result():
    data = request.json
    print(f"[DEBUG] /auth_result hit with data: {data}")

    if data['recognized']:
        user_id = data['user_id']
        print(f"[INFO] Recognized user: {user_id}")
        token = generate_token(user_id)

        # Log the token here
        print(f"[INFO] Generated token for user {user_id}: {token}")

        socketio.emit('auth_token', {'token': token})
    else:
        print("[INFO] Face not recognized")
        socketio.emit('auth_timeout', {'message': 'Face not recognized. Please try again.'})

    return '', 200

@app.route('/dashboard')
def dashboard():
    token = request.args.get('token') or session.get('token')
    # print(f"[DEBUG] /dashboard accessed with token: {token}")
    if not token:
        return redirect('/')
    
    user_id = validate_token(token)
    if not user_id:
        return redirect('/')
    
    return render_template('home.html', user_id=user_id)

# ---------------- SOCKET EVENTS ---------------- #
@socketio.on('connect')
def handle_connect():
    print('Client connected')

@socketio.on('disconnect')
def handle_disconnect():
    # Clean up any pending auth requests for this session
    auth_requests.pop(request.sid, None)
    print(f'Client disconnected: {request.sid}')

@socketio.on('request_auth')
def handle_auth_request(data):
    print("Auth requested:", data)
    auth_id = str(uuid.uuid4())
    auth_requests[auth_id] = {
        'session_id': request.sid,
        'timestamp': datetime.utcnow(),
        'status': 'pending'
    }

    try:
        notify_esp32_serial(auth_id)
        print("[INFO] Sent recognition trigger to ESP32 via Serial")
    except Exception as e:
        print(f"[ERROR] Could not send start command to ESP32: {e}")

    emit('auth_requested', {
        'auth_id': auth_id,
        'message': 'Please look at the ESP32 camera for authentication'
    })

@socketio.on('verify_token')
def handle_verify_token(data):
    """Frontend sends the token for verification"""
    token = data.get('token')
    user_id = validate_token(token)
    if user_id:
        emit('auth_verified', {'user_id': user_id})
    else:
        emit('auth_failed', {'message': 'Invalid or expired token'})

# ---------------- ESP32 SERIAL ---------------- #
def notify_esp32_serial(auth_id):
    try:
        ser = serial.Serial('COM6', 115200, timeout=5)
        time.sleep(2)
        command = f"START:{auth_id}\n"
        ser.write(command.encode())
        print(f"[INFO] Sent to ESP32: {command.strip()}")
        response = ser.readline().decode().strip()
        print(f"[ESP32 RESPONSE] {response}")
        ser.close()
    except Exception as e:
        print(f"[ERROR] Could not write to ESP32: {e}")

# ---------------- MAIN ---------------- #
if __name__ == '__main__':
    cleaner_thread = threading.Thread(target=clean_expired_tokens, daemon=True)
    cleaner_thread.start()
    
    print("Running on http://0.0.0.0:5000 (no SSL)")
    socketio.run(app, host='0.0.0.0', port=5000)
