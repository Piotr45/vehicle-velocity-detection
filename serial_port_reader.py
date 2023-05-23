"""Simple video stream writer."""

import argparse
import datetime
import serial
import sys
import os

from typing import List


def parse_arguments(argv: List[str]) -> argparse.Namespace:
    arg_parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )

    arg_parser.add_argument(
        "--port",
        type=str,
        action="store",
        required=True,
        help="Serial port eg. COM3",
    )

    arg_parser.add_argument(
        "--baudrate",
        type=str,
        action="store",
        required=True,
        help="Baudrate eg. 921600",
    )

    arg_parser.add_argument(
        "--output-dir",
        type=str,
        action="store",
        required=True,
        help="Output dir",
    )

    arg_parser.add_argument(
        "--pics",
        type=int,
        action="store",
        required=True,
        help="Output dir",
    )

    return arg_parser.parse_args(argv)


def main() -> None:
    # parse arguments
    args = parse_arguments(sys.argv[1:])
    port = args.port
    baudrate = args.baudrate
    output_dir = args.output_dir
    max_pics = args.pics

    # init serial port
    serial_port = serial.Serial(port, baudrate=baudrate)

    if not os.path.isdir(output_dir):
        os.mkdir(output_dir)

    pics = 0
    # loop receiving args.pics jpeg files
    while pics < max_pics:

        # make filename for new jpeg file
        filename = f"{datetime.datetime.today().strftime('%Y-%m-%d_%H-%M-%S')}.jpg"
        if pics > 0:
            file = open(os.path.join(output_dir, filename), "wb")

        # assumes first byte received is start of jpeg header FFD8
        data_last = 0
        while 1:  # this needed b/c serial seems to be buffered on sent/receipt
            if serial_port.inWaiting() > 0:
                data = serial_port.read()
                if pics > 0:
                    file.write(data)
                if data_last == b"\xFF" and data == b"\xD9":
                    break  # exit loop and close file on jpeg footer FFD9
                data_last = data

        # close file
        if pics > 0:
            file.close()
        pics += 1


if __name__ == "__main__":
    main()
