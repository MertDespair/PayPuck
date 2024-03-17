from flask import Flask, render_template, request, redirect, url_for, jsonify
from datetime import datetime, timedelta
import json, os


app = Flask(__name__)

# Define folder path
folder_path = "configuration"

# Define file paths for JSON files within the folder
user_json_path = os.path.join(folder_path, "users.json")
drinks_json_path = os.path.join(folder_path, "drinks.json")

with open(user_json_path, 'r') as user_file:
    user_list = json.load(user_file)

with open(drinks_json_path, 'r') as drinks_file:
    drinks = json.load(drinks_file)

#Rechnungs Daten
current_invoice_number = 1
from_addr = {
    'company_name': "PayPuk GmbH",
    'addr1': "Musterstraße 45",
    'addr2': "12345 Musterstadt"
}
iban = "DE02 5001 0517 9343 9317 83"


@app.route('/')
def index():
    return redirect(url_for('login'))

@app.route('/login', methods=['GET', 'POST'])
def login():
    if request.method == 'POST':
        username = request.form['username']
        password = request.form['password']
        user = next((item for item in user_list if item['username'] == username), None)
        if user != None:
            if (user['password'] == password): return redirect(url_for('dashboard', username=username))
            else: return render_template('username.html', error='Falsches Password')
        else:
            return render_template('username.html', error='Username exestiert nicht')
    return render_template('username.html')

@app.route('/dashboard/<username>', methods=['GET', 'POST'])
def dashboard(username):
    if request.method == 'POST':
        return redirect(url_for('dashboard', username=username))
    return render_template('dashboard.html', username=username)

@app.route('/data', methods=['POST'])
def receive_puk_data():
    data = request.json
    print("Received data:", data)
    if(data['bar_paypuk'] == '0'):
        print("Booking")
        esp32_id = data['esp_id']
        user = next((item for item in user_list if item['puk_key'] == esp32_id), None)
        drink = next((item for item in drinks if isinstance(item['IDs'], list) and data['rfid_id'] in item['IDs']), None)
        user_drink = {'name': drink['name'], 'price': drink['price'], 'rfid_id': data['rfid_id'], 'timestamp': datetime.now().strftime("%d.%m.%Y %H:%MUhr")}
        if(user != None and drink != None and (next((user['username'] for user in user_list if any(drink['rfid_id'] == data['rfid_id'] for drink in user['open_drinks'])), None)) == None):
            user['open_drinks'].append(user_drink)
        else:
            #Code wenn Getränk nicht gebucht werden kann
            pass
        print(user)
        return 'Data received successfully'
    else:
        matching_drink = next((drink for user in user_list for drink in user['open_drinks'] if drink['rfid_id'] == data['rfid_id']), None)
        print(matching_drink)
        if(matching_drink != None): matching_drink['rfid_id'] = None
        print(matching_drink)
        return 'Data received successfully'
    




@app.route('/puk_id_data', methods=['POST'])
def receive_puk_id():
    data = request.json
    print("Received data:", data)
    user = next((item for item in user_list if item['username'] == data['username']), None)
    if(next((item for item in user_list if item['puk_key'] == data['esp_id']), None) == None or data['esp_id'] == None):
        user['puk_key'] = data['esp_id']
    else:
        #Puk ID schon vergeben
        pass
    print(user)
    return 'Data received successfully'

@app.route('/pay_bill', methods=['POST'])
def pay():
    global current_invoice_number
    data = request.json
    user = next((item for item in user_list if item['username'] == data['username']), None)
    if user['open_drinks']:
        date_today = datetime.today()
        due_date = date_today + timedelta(days=30)
        invoice = render_template('invoice.html',
                            date=date_today.strftime('%d.%m.%Y'),
                            from_addr=from_addr,
                            to_addr=user['address'],
                            items=user['open_drinks'],
                            total = sum(item['price'] for item in user['open_drinks']),
                            invoice_number=str(current_invoice_number).zfill(8),
                            duedate=due_date.strftime('%d.%m.%Y'),
                            iban = iban)
        user['open_drinks'] = []
        current_invoice_number +=  1
        print(user)
        return invoice
    else:
        #Keine Getränke
        return None

@app.route('/get_data/<username>', methods=['GET'])
def get_data(username):
    # Die gespeicherten Daten für den angegebenen ESP32-ID zurückgeben
    user = next((item for item in user_list if item['username'] == username), None)
    return jsonify(user)



if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
