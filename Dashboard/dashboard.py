# =================================================================
# Importação de Bibliotecas
# =================================================================
import dash
from dash import dcc, html
from dash.dependencies import Input, Output
import plotly.graph_objs as go
import requests
from datetime import datetime
import pytz

# =================================================================
# CONFIGURAÇÃO
# =================================================================
IP_ADDRESS = "20.151.77.156"
PORT_STH = 8666
ENTITY_ID = "urn:ngsi-ld:RoomMonitor:101"
ENTITY_TYPE = "RoomMonitor"
DASH_HOST = "0.0.0.0"
DASH_PORT = 5000
LAST_N_RECORDS = 50

# =================================================================
# FUNÇÕES AUXILIARES
# =================================================================

def get_historical_data(attribute: str, lastN: int) -> list:
    url = f"http://{IP_ADDRESS}:{PORT_STH}/STH/v1/contextEntities/type/{ENTITY_TYPE}/id/{ENTITY_ID}/attributes/{attribute}?lastN={lastN}"
    headers = {
        'fiware-service': 'smart',
        'fiware-servicepath': '/'
    }
    try:
        response = requests.get(url, headers=headers, timeout=10)
        if response.status_code == 200:
            data = response.json()
            values = data['contextResponses'][0]['contextElement']['attributes'][0]['values']
            return values
        else:
            print(f"Erro ao aceder a {url}: Status {response.status_code}")
            return []
    except requests.exceptions.RequestException as e:
        print(f"Erro de conexão ao aceder a {url}: {e}")
        return []
    except (KeyError, IndexError) as e:
        print(f"Erro ao processar os dados JSON para o atributo '{attribute}': {e}")
        return []

def convert_to_sao_paulo_time(timestamps_utc: list) -> list:
    utc = pytz.utc
    sao_paulo_tz = pytz.timezone('America/Sao_Paulo')
    converted_timestamps = []
    for ts_str in timestamps_utc:
        try:
            ts_str_clean = ts_str.replace('T', ' ').replace('Z', '')
            try:
                dt_utc = datetime.strptime(ts_str_clean, '%Y-%m-%d %H:%M:%S.%f')
            except ValueError:
                dt_utc = datetime.strptime(ts_str_clean, '%Y-%m-%d %H:%M:%S')

            converted_timestamps.append(utc.localize(dt_utc).astimezone(sao_paulo_tz))
        except Exception as e:
            print(f"Não foi possível converter o timestamp '{ts_str}': {e}")
    return converted_timestamps

# =================================================================
# INICIALIZAÇÃO E LAYOUT DO DASHBOARD
# =================================================================

app = dash.Dash(__name__)

app.layout = html.Div(className='app-container', children=[

    # Cabeçalho simplificado com apenas o título
    html.Div(className='header', children=[
        html.H1('Monitoramento do Armazém')
    ]),

    # O contentor para os nossos três gráficos
    html.Div(className='graphs-container', children=[
        html.Div(dcc.Graph(id='temperature-graph'), className='graph-item-wide'),
        html.Div(dcc.Graph(id='humidity-graph'), className='graph-item'),
        html.Div(dcc.Graph(id='luminosity-graph'), className='graph-item'),
    ]),

    # Componentes invisíveis que permanecem
    dcc.Interval(id='interval-component', interval=10 * 1000, n_intervals=0),
    dcc.Store(id='temperature-store'),
    dcc.Store(id='humidity-store'),
    dcc.Store(id='luminosity-store'),
])
# =================================================================
# CALLBACKS
# =================================================================

def create_data_update_callback(attribute: str):
    @app.callback(Output(f'{attribute}-store', 'data'), Input('interval-component', 'n_intervals'))
    def update_data_store(n_intervals):
        data = get_historical_data(attribute, LAST_N_RECORDS)
        if not data:
            return {'timestamps': [], 'values': []}

        timestamps = [entry['recvTime'] for entry in data]
        values = [float(entry['attrValue']) for entry in data]
        converted_times = convert_to_sao_paulo_time(timestamps)
        return {'timestamps': converted_times, 'values': values}

def create_graph_update_callback(title: str, yaxis_title: str, color: str):
    @app.callback(Output(f'{title.lower()}-graph', 'figure'), Input(f'{title.lower()}-store', 'data'))
    def update_graph(stored_data):
        layout = {
            'title': f'{title}', 'xaxis_title': 'Ho´rario (São Paulo)', 'yaxis_title': yaxis_title,
            'height': 350,
            'hovermode': 'x unified', 'plot_bgcolor': '#34495e', 'paper_bgcolor': '#2c3e50',
            'font': {'color': '#ecf0f1'}, 'xaxis': {'gridcolor': '#555'}, 'yaxis': {'gridcolor': '#555'}
        }
        if not stored_data or not stored_data['timestamps']:
            return go.Figure(layout=layout)

        timestamps = stored_data['timestamps']
        values = stored_data['values']
        mean_value = sum(values) / len(values)
        trace_values = go.Scatter(x=timestamps, y=values, mode='lines+markers', name=yaxis_title, line={'color': color})
        trace_mean = go.Scatter(x=[timestamps[0], timestamps[-1]], y=[mean_value, mean_value], mode='lines', name=f'Média ({mean_value:.2f})', line={'color': 'white', 'dash': 'dash'})
        return go.Figure(data=[trace_values, trace_mean], layout=layout)

create_data_update_callback('temperature')
create_data_update_callback('humidity')
create_data_update_callback('luminosity')

# Cores escolhidas para melhor contraste e legibilidade
create_graph_update_callback('Temperature', 'Temperatura (°C)', '#e74c3c')      # Vermelho para calor
create_graph_update_callback('Humidity', 'Umidade (%)', '#3498db')          # Azul para água/umidade
create_graph_update_callback('Luminosity', 'Luminosidade (%)', '#2ecc71')      # Verde esmeralda para luz

# =================================================================
# Execução do Servidor Web
# =================================================================
if __name__ == '__main__':
    print(f"Dashboard a iniciar. Aceda em: http://{DASH_HOST}:{DASH_PORT}")
    app.run(debug=True, host=DASH_HOST, port=DASH_PORT)
