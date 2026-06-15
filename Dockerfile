FROM python:3.12-slim

WORKDIR /src
COPY . .

RUN python -m pip install --no-cache-dir build \
    && python -m build

CMD ["sh", "-c", "cp -v dist/* /host-dist/"]
