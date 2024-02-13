
from flask import Flask, jsonify, render_template, request

app = Flask(__name__)

# This dictionary will store your sensor data
sensor_data = {
    'A': {'level': 0, 'volume': 0},
    'B': {'level': 0, 'volume': 0},
    'C': {'level': 0, 'volume': 0}
}

@app.route('/')
def index():
    # Pass sensor data to the HTML template
    return render_template('index.html', sensor_data=sensor_data)

@app.route('/data')
def data():
    # Return sensor data in JSON format
    return jsonify(sensor_data)

@app.route('/update', methods=['POST'])
def update():
    # This route accepts JSON data and updates the sensor_data dictionary
    data = request.get_json()
    for tank_id, tank_values in data.items():
        sensor_data[tank_id]['level'] = tank_values['level']
        sensor_data[tank_id]['volume'] = tank_values['volume']
    return 'Data updated successfully', 200

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0')