from aiohttp import web
import json
import re
import datetime
import random
from hashlib import sha1
from typing import List, Union, Dict


class JsonResponse(web.Response):
    def __init__(self, json_data: Union[Dict, List], status: int = 200):
        super(JsonResponse, self).__init__(
            body=json.dumps(json_data),
            headers = {
                "Content-Type": "application/json"
            },
            status=status,
        )


class Server(web.Application):
    # https://regex101.com/r/cLMEBd/1
    re_seed = re.compile(r"^[a-fA-F0-9]{32}$")

    salt = b"uuuu"

    known_devices = []

    def __init__(self):
        super().__init__()
        self.add_routes([
            web.get('/getToken', self.provisionning),
            web.get('/getDevices', self.get_devices),
            web.get('/{uid}', self.handle)
        ])

    # concatenation of seed with timestamp with "uuuu"
    def gen_token(self, seed: str, timestamp: int) -> str:
        return sha1(bytes.fromhex(seed) + timestamp.to_bytes(4, "little") + self.salt).hexdigest()

    # /GetToken?Seed=258e3f3bd8a4a54aabcf5ef6723effdd
    async def provisionning(self, request: web.Request) -> web.Response:
        seed = request.query.get("Seed", "")
        match = Server.re_seed.match(seed)
        if seed and match:
            now = datetime.datetime.now()
            timestamp = int(now.timestamp())

            data = {
                "Datetime": now.strftime("%Y/%m/%d %H:%M:%S"),
                "Timestamp": timestamp,
                "Seed": seed,
                "Salt": self.salt.decode("utf8"),
                "Token": self.gen_token(seed, timestamp),
            }
            self.known_devices.append(data)
            return JsonResponse(json_data=data, status=200)

        return web.Response(status=404)

    async def get_devices(self, request: web.Request):
        return JsonResponse(json_data=self.known_devices, status=200)

    async def handle(self, request: web.Request):
        name = request.match_info.get('name', "Anonymous")
        text = request.url.human_repr() + " : Hello " + name
        print(text)
        return web.Response(text=text)


if __name__ == '__main__':
    web.run_app(Server(), port=8080)