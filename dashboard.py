from datetime import datetime

import streamlit as st
import pandas as pd

@st.fragment(run_every="1s")
def display_data():

    if not st.session_state.raw_data:
        return
    
    df = pd.DataFrame(st.session_state.raw_data)
        
    if "time" in df.columns:
        df = df.set_index("time")
    
    current_moisture = df["soil_moisture"].iloc[-1]
    current_temp = df["temperature"].iloc[-1]
    current_status = df["status"].iloc[-1]

    m1, m2, m3 = st.columns(3)
    m1.metric("상태", current_status)
    m2.metric("토양 수분", current_moisture)
    m3.metric("현재 온도", current_temp)


    plot_moisture = df[["soil_moisture"]].tail(60).copy()
    
    plot_moisture["soil_moisture_ma"] = plot_moisture["soil_moisture"].rolling(window=10).mean() 
    plot_moisture["soil_moisture_ma"] = plot_moisture["soil_moisture_ma"].fillna(plot_moisture["soil_moisture"])

    plot_moisture = plot_moisture.rename(columns={
        "soil_moisture": "토양 수분",
        "soil_moisture_ma": "이동 평균"
    })
    plot_moisture = plot_moisture[["토양 수분", "이동 평균"]]

    st.line_chart(
        plot_moisture,
        color=["#BA9C86", "#BB5206"]
    )

    current_value = plot_moisture["토양 수분"].values[-1]
    max_value = plot_moisture["토양 수분"].max()
    min_value = plot_moisture["토양 수분"].min()
    avg_value = plot_moisture["토양 수분"].mean()

    m1, m2, m3, m4 = st.columns(4)
    m1.metric("현재", current_value)
    m2.metric("최대", max_value)
    m3.metric("최소", min_value)
    m4.metric("평균", f"{avg_value:0.0f}")

    st.divider()
    
    plot_temperature = df.tail(60).copy()
    plot_temperature = plot_temperature[["temperature"]]
    
    plot_temperature = plot_temperature.rename(columns={"temperature": "온도"})

    # 사용한 색상 테마
    # https://coolors.co/palette/ef476f-ffd166-06d6a0-118ab2-073b4c
    st.line_chart(
        plot_temperature
    )
    
    current_value = plot_temperature["온도"].values[-1]
    max_value = plot_temperature["온도"].max()
    min_value = plot_temperature["온도"].min()
    avg_value = plot_temperature["온도"].mean()

    m1, m2, m3, m4 = st.columns(4)
    m1.metric("현재", current_value)
    m2.metric("최대", max_value)
    m3.metric("최소", min_value)
    m4.metric("평균", f"{avg_value:0.0f}")

    st.divider()
    
    with st.expander("원본 데이터 보기"):
        st.dataframe(df.sort_index(ascending=False))

display_data()