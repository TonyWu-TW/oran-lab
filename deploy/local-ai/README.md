# Local Qwen + Open WebUI

This deployment provides:

- Qwen3.5-9B through a vLLM OpenAI-compatible API
- Open WebUI Slim as a local ChatGPT-style interface

The preinstalled vLLM runtime image includes the Qwen3.5 backend even though
its local Docker tag is named `gemma4-unified`.

## Start

```bash
cd /home/zju/Desktop/oran-lab/deploy/local-ai
docker compose up -d
```

The first model start can take several minutes. Check progress with:

```bash
docker compose logs -f vllm
```

When both services are ready:

- Web UI: http://10.106.133.244 (preferred)
- Alternate Web UI port: http://10.106.133.244:3000
- vLLM API: http://127.0.0.1:8000/v1
- Model name: `qwen3.5-9b`
- Maximum context length: 32,768 tokens

## Status

```bash
docker compose ps
curl http://127.0.0.1:8000/v1/models
curl http://10.106.133.244:3000/health
```

## Stop or restart

```bash
docker compose stop
docker compose restart
```

The containers use `restart: unless-stopped`, so they start again after a
normal machine reboot unless they were explicitly stopped first.

Chat history is retained in the `oran-local-ai_open-webui-data` Docker volume.
Model files are retained under
`/home/zju/.cache/oran-local-ai/models/Qwen3.5-9B`.

The deployment is intentionally chat-only. Open WebUI document
embedding/retrieval is bypassed so it does not download or run a second model.

## NVIDIA driver update

If `nvidia-smi` reports `Driver/library version mismatch`, reboot once so the
running kernel module matches the installed NVIDIA libraries:

```bash
sudo reboot
```

After logging back in, start the stack and verify the API:

```bash
cd /home/zju/Desktop/oran-lab/deploy/local-ai
docker compose up -d
docker compose logs -f vllm
curl http://127.0.0.1:8000/v1/models
```
