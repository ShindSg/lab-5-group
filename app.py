import json
import socket
import time
import streamlit as st

SERVER_HOST = "127.0.0.1"
SERVER_PORT = 8080

st.set_page_config(page_title="Stack Overflow Search", layout="wide")
st.title("Stack Overflow Search Engine")
st.caption("Инвертированный поисковый индекс через TCP Сетевой Демон (C бэкенд)")

col_left, col_right = st.columns([3, 1])

with col_left:
    query = st.text_input("Поисковый запрос", placeholder="python list sort")

with col_right:
    tree_type = st.selectbox("Структура данных индекса", ["avl", "rb", "btree"])

def send_query_to_server(tree_type, query_text):
    payload = {
        "tree_type": tree_type,
        "query": query_text
    }
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(10.0)
        s.connect((SERVER_HOST, SERVER_PORT))
        s.sendall(json.dumps(payload).encode('utf-8'))
        
        response_bytes = b""
        while True:
            chunk = s.recv(8192)
            if not chunk:
                break
            response_bytes += chunk
            if b"\n" in chunk:
                break
        s.close()
        return json.loads(response_bytes.decode('utf-8'))
    except Exception as e:
        return {"status": "error", "message": f"Ошибка соединения с C-сервером: {e}"}

if query:
    with st.spinner("Запрос обрабатывается C-сервером..."):
        t0 = time.monotonic()
        data = send_query_to_server(tree_type, query)
        wall_ms = (time.monotonic() - t0) * 1000

    if data.get("status") == "error" or "results" not in data:
        st.error(data.get("message", "Неверный формат ответа от сервера."))
    else:
        total = data.get("total", 0)
        idx_ms = data.get("time_ms", wall_ms)
        results = data.get("results", [])

        st.write(f"**Найдено результатов:** {total} | **Время поиска:** {idx_ms:.2f} мс (Сеть + Рендеринг: {wall_ms:.2f} мс)")

        if not results:
            st.info("По вашему запросу ничего не найдено.")
        else:
            for i, r in enumerate(results[:10], 1):
                with st.expander(f"{i}. {r.get('title')} (ID документа: {r.get('doc_id')})"):
                    st.write(f"**Вычисленный Score релевантности:** {r.get('score')}")