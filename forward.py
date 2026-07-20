#!/usr/bin/python3
#
# Copyright (c) 2025 secfurry
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

from os import makedirs
from struct import pack
from io import BytesIO, StringIO
from sys import stderr, exit, argv
from argparse import ArgumentParser
from telethon import functions
from telethon.sync import TelegramClient
from telethon.tl.types import PeerUser, PeerChannel, PeerChat
from telethon.tl.types import Channel, MessageMediaDocument, MessageMediaPhoto, MessageMediaWebPage, MessageMediaPoll, MessageFwdHeader, MessageMediaContact, MessageMediaPaidMedia
from telethon.errors.rpcerrorlist import ChatForwardsRestrictedError
from os.path import join, exists, isdir, expanduser, expandvars
import time
import random
from datetime import datetime
import requests
import traceback


t_doc, t_photo, t_webpage, t_protect = set(), set(), set(), set()
total_send = 0
flood_limit = 1500


def _main():
    p = ArgumentParser(add_help=False)
    p.add_argument(
        "-s",
        "--state",
        type=str,
        dest="state",
        action="store",
        default="deduper",
        required=False,
    )
    p.add_argument(
        "-n",
        "--name",
        type=str,
        dest="name",
        action="store",
        required=False,
    )
    p.add_argument(
        "-p",
        "--password",
        type=str,
        dest="password",
        action="store",
        required=False,
    )
    p.add_argument(
        "-t",
        "--totp",
        type=str,
        dest="totp",
        action="store",
        required=False,
    )
    p.add_argument(
        "-c",
        "--channel",
        type=str,
        dest="channel",
        action="store",
        required=True,
    )
    p.add_argument(
        "-i",
        "--app-id",
        type=int,
        dest="app_id",
        action="store",
        required=True,
    )
    p.add_argument(
        "-h",
        "--app-hash",
        type=str,
        dest="app_hash",
        action="store",
        required=True,
    )
    a = p.parse_args()
    del p
    if not isinstance(a.app_hash, str) or len(a.app_hash) == 0:
        raise ValueError("app_hash cannot be empty")
    s = a.state


    with open('document.txt', 'r') as myfile:
        for t_id in myfile.read().splitlines():
            t_doc.add(int(t_id))

    with open('photo.txt', 'r') as myfile:
        for t_id in myfile.read().splitlines():
            t_photo.add(int(t_id))

    with open('webpage.txt', 'r') as myfile:
        for t_id in myfile.read().splitlines():
            t_webpage.add(int(t_id))

    with open('protect.txt', 'r') as myfile:
        for line in myfile.read().splitlines():
            t_protect.add(line)


    client = TelegramClient(s, a.app_id, a.app_hash)
    client.connect()

    if not client.is_user_authorized():
        if a.name == None:
            client.start()

        else:
            client.send_code_request(a.name)

            print('code', flush=True)
            code = None

            while True:
                try:
                    if a.totp != None:
                        response = requests.get(a.totp)
                        if response.status_code == 200:
                            code = response.text
                    else:                        
                        with open('code.txt', 'r') as myfile:
                            code = (myfile.read().splitlines())[0]

                    if code == None:
                        time.sleep(1)
                        continue
                    break

                except:
                    time.sleep(1)

            try:
                client.sign_in(a.name, code)
            except:
                client.sign_in(password=a.password)

    client.get_dialogs()


    out_d = list()

    with open('channels.txt', 'r', encoding='utf-8') as myfile:
        for line in myfile.read().splitlines():
            try:
                if total_send >= flood_limit:       # Flood warning
                    out_d.append(line)
                    continue

                args = line.split('\t')
                print(datetime.now())
                args[1] = _forward(client, int(args[0]), args[1], int(a.channel))

                out_d.append(str(args[0]) + '\t' + str(args[1]) + '\t' + str(args[2]))
                continue

            except Exception as e:
                print(e)
                traceback.print_exc()


    with open('channels.txt', 'w', encoding='utf-8') as myfile:
        for line in out_d:
            print(line, file=myfile)

    with open('document.txt', 'w') as myfile:
        for line in t_doc:
            print(line, file=myfile)  # Python 3.x

    with open('photo.txt', 'w') as myfile:
        for line in t_photo:
            print(line, file=myfile)  # Python 3.x

    with open('webpage.txt', 'w') as myfile:
        for line in t_webpage:
            print(line, file=myfile)  # Python 3.x

    with open('protect.txt', 'w') as myfile:
        for line in t_protect:
            print(line, file=myfile)  # Python 3.x


def _forward(client, channel_id, start_id, dest_id):
    global total_send

    last_id = int(start_id)
    flood = False

    COVER_BOT_USERNAMES = { 5605632845, 432668551, 7210051051, 7076686029 }  # forwardcoverbot new, forwardscoverbot, simple forward cover bot, Forwards Cover KR Bot

    try:
        o = 0
        for i in client.iter_messages(channel_id, reverse=True, offset_id=int(start_id)):
            o += 1
            if o % 100 == 0:
                print(o, flush=True)

            time.sleep(random.uniform(0.001, 0.01))
            last_id = int(i.id)


            topic_id = 9530   # File

            # print(i)
            # print(i.media)


            if not i.media:
                if not i.message:
                    continue
                if not 'https://' in i.message:
                    continue
                topic_id = 9529

            elif type(i.media) is MessageMediaDocument:
                if not i.media.document.id or int(i.media.document.id) in t_doc:
                    continue
                if i.media.video == True:
                    topic_id = 9532

            elif type(i.media) is MessageMediaPhoto:
                if not i.media.photo.id or int(i.media.photo.id) in t_photo:
                    continue
                topic_id = 9531

            elif type(i.media) is MessageMediaWebPage:
                if not i.media.webpage.id or int(i.media.webpage.id) in t_webpage:
                    continue
                topic_id = 9529

            elif type(i.media) is MessageMediaPoll:
                continue

            elif type(i.media) is MessageMediaContact:
                continue

            elif type(i.media) is MessageMediaPaidMedia:
                continue


            for bot_username in COVER_BOT_USERNAMES:
                if i.forward and i.forward.sender_id == bot_username:
                    topic_id = 142078


            if i.noforwards == True:
                t_protect.add(str(channel_id) + '\t' + str(i.id))
                continue


            if total_send >= flood_limit:       # Flood warning
                last_id -= 1
                print('Flood control', flush=True)
                break


            time.sleep(random.uniform(0.075, 0.20))     # Flood warning  (20 > sec)
            # i.forward_to(dest_id)


            flood = True

            try:
                client(functions.messages.ForwardMessagesRequest(
                    from_peer=i.peer_id,
                    id=[i.id],
                    to_peer=dest_id,
                    top_msg_id=topic_id,
                ))
            except ChatForRestrictedError:
                print("Content in this chat is protected.")
                return
    
            flood = False


            if type(i.media) is MessageMediaDocument:
                t_doc.add(int(i.media.document.id))

            elif type(i.media) is MessageMediaPhoto:
                t_photo.add(int(i.media.photo.id))

            elif type(i.media) is MessageMediaWebPage:
                t_webpage.add(int(i.media.webpage.id))

            total_send += 1

    except Exception as e:
        print(e)
        traceback.print_exc()

        last_id -= 1

        if flood == True:
            total_send = flood_limit


    return last_id


if __name__ == "__main__":
    try:
        _main()
    except Exception as err:
        print(f"Error during runtime: {err}!", file=stderr)
        traceback.print_exc()
        exit(1)
