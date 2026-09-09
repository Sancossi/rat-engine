"""Small real MCP CLI, using official Python mcp==2.2.0 (install separately)."""
import argparse
import asyncio
import base64
import json
from pathlib import Path
from mcp import Client
from mcp.client.stdio import StdioServerParameters


async def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--server", required=True)
    parser.add_argument("--connection", required=True)
    parser.add_argument("--tool")
    parser.add_argument("--arguments", default="{}")
    parser.add_argument("--arguments-file")
    parser.add_argument("--output", required=True)
    options = parser.parse_args()
    output = Path(options.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    async with Client(StdioServerParameters(command=options.server, args=["--connection", options.connection]), read_timeout_seconds=30) as client:
        arguments = json.loads(Path(options.arguments_file).read_text(encoding="utf-8-sig") if options.arguments_file else options.arguments)
        result = await client.call_tool(options.tool, arguments) if options.tool else await client.list_tools()
        value = result.model_dump(mode="json")
        for index, content in enumerate(value.get("content", [])):
            if content["type"] == "image":
                image = output.with_suffix(f".{index}.png")
                image.write_bytes(base64.b64decode(content.pop("data")))
                content["savedImage"] = str(image)
        output.write_text(json.dumps({"protocolVersion": client.protocol_version, "result": value}, ensure_ascii=False, indent=2), encoding="utf-8")
        print(output)


if __name__ == "__main__":
    asyncio.run(main())
