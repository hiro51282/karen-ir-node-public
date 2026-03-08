FROM python:3.11

RUN pip install platformio

WORKDIR /workspace

CMD ["pio", "run"]