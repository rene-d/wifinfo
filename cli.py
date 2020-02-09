#!/usr/bin/env python3
#
# Very simple serial terminal
#
# This file is part of pySerial. https://github.com/pyserial/pyserial
# (C)2002-2015 Chris Liechti <cliechti@gmx.net>
#
# SPDX-License-Identifier:    BSD-3-Clause

import codecs
import sys
import threading
import datetime
import atexit
import termios
import fcntl
import serial
from serial.tools import hexlify_codec
import os.path
import time

# pylint: disable=wrong-import-order,wrong-import-position

codecs.register(lambda c: hexlify_codec.getregentry() if c == "hexlify" else None)


hchc = 15000000
hchp = 10000000
iinst = 670 / 230 +3
tic_time = time.time()
heures_pleines = True


def group(label, value):
    sum = 0
    for c in label:
        sum += ord(c)
    sum += ord(" ")
    for c in value:
        sum += ord(c)
    sum = (sum & 63) + 32
    return "\x0A" + label + " " + str(value) + " " + chr(sum) + "\x0D"


def tic():
    global tic_time, hchp, hchc

    now = time.time()
    delta = (now - tic_time) * iinst * 230 / 3600
    tic_time = now

    papp = int(iinst * 230)

    if heures_pleines:
        ptec = "HP.."
        hchp += delta
    else:
        ptec = "HC.."
        hchc += delta

    frame = "\x02"
    frame += group("ADCO", "040522079986")
    frame += group("OPTARIF", "HC..")
    frame += group("ISOUSC", "30")
    frame += group("HCHC", f"{int(hchc):09d}")
    frame += group("HCHP", f"{int(hchp):09d}")
    frame += group("PTEC", ptec)
    frame += group("IINST", f"{int(iinst):03d}")
    frame += group("IMAX", "042")
    frame += group("PAPP", f"{papp:05d}")
    frame += group("HHPHC", "D")
    frame += group("MOTDETAT", "000000")
    frame += "\x03"

    return frame.encode("ascii")


class Periodic(threading.Thread):
    """ timer périodique """

    def __init__(self, interval, function):
        self.interval = interval
        self.function = function
        threading.Thread.__init__(self)
        self.stopped = threading.Event()

    def stop(self):
        self.stopped.set()

    def run(self):
        while not self.stopped.wait(self.interval):
            self.function()


# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class ConsoleBase(object):
    """OS abstraction for console (input/output codec, no echo)"""

    def __init__(self):
        if sys.version_info >= (3, 0):
            self.byte_output = sys.stdout.buffer
        else:
            self.byte_output = sys.stdout
        self.output = sys.stdout

    def setup(self):
        """Set console to read single characters, no echo"""

    def cleanup(self):
        """Restore default console settings"""

    def getkey(self):
        """Read a single key from the console"""
        return None

    def write_bytes(self, byte_string):
        """Write bytes (already encoded)"""
        self.byte_output.write(byte_string)
        self.byte_output.flush()

    def write(self, text):
        """Write string"""
        self.output.write(text)
        self.output.flush()

    def cancel(self):
        """Cancel getkey operation"""

    #  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
    # context manager:
    # switch terminal temporary to normal mode (e.g. to get user input)

    def __enter__(self):
        self.cleanup()
        return self

    def __exit__(self, *args, **kwargs):
        self.setup()


class Console(ConsoleBase):
    def __init__(self):
        super(Console, self).__init__()
        self.fd = sys.stdin.fileno()
        self.old = termios.tcgetattr(self.fd)
        atexit.register(self.cleanup)
        if sys.version_info < (3, 0):
            self.enc_stdin = codecs.getreader(sys.stdin.encoding)(sys.stdin)
        else:
            self.enc_stdin = sys.stdin

    def setup(self):
        new = termios.tcgetattr(self.fd)
        new[3] = new[3] & ~termios.ICANON & ~termios.ECHO & ~termios.ISIG
        new[6][termios.VMIN] = 1
        new[6][termios.VTIME] = 0
        termios.tcsetattr(self.fd, termios.TCSANOW, new)

    def getkey(self):
        c = self.enc_stdin.read(1)
        if c == chr(0x7F):
            c = chr(8)  # map the BS key (which yields DEL) to backspace
        return c

    def cancel(self):
        fcntl.ioctl(self.fd, termios.TIOCSTI, b"\0")

    def cleanup(self):
        termios.tcsetattr(self.fd, termios.TCSAFLUSH, self.old)


# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


class Transform(object):
    """do-nothing: forward all data unchanged"""

    def rx(self, text):
        """text received from serial port"""
        return text

    def tx(self, text):
        """text to be sent to serial port"""
        return text

    def echo(self, text):
        """text to be sent but displayed on console"""
        return text


class CRLF(Transform):
    """ENTER sends CR+LF"""

    def tx(self, text):
        return text.replace("\n", "\r\n")


class CR(Transform):
    """ENTER sends CR"""

    def rx(self, text):
        return text.replace("\r", "\n")

    def tx(self, text):
        return text.replace("\n", "\r")


class LF(Transform):
    """ENTER sends LF"""


class NoTerminal(Transform):
    """remove typical terminal control codes from input"""

    REPLACEMENT_MAP = dict((x, 0x2400 + x) for x in range(32) if chr(x) not in "\r\n\b\t")
    REPLACEMENT_MAP.update(
        {0x7F: 0x2421, 0x9B: 0x2425,}  # DEL  # CSI
    )

    def rx(self, text):
        return text.translate(self.REPLACEMENT_MAP)

    echo = rx


class NoControls(NoTerminal):
    """Remove all control codes, incl. CR+LF"""

    REPLACEMENT_MAP = dict((x, 0x2400 + x) for x in range(32))
    REPLACEMENT_MAP.update(
        {0x20: 0x2423, 0x7F: 0x2421, 0x9B: 0x2425,}  # visual space  # DEL  # CSI
    )


class Printable(Transform):
    """Show decimal code for all non-ASCII characters and replace most control codes"""

    def rx(self, text):
        r = []
        for c in text:
            if " " <= c < "\x7f" or c in "\r\n\b\t":
                r.append(c)
            elif c < " ":
                r.append(chr(0x2400 + ord(c)))
            else:
                r.extend(chr(0x2080 + ord(d) - 48) for d in "{:d}".format(ord(c)))
                r.append(" ")
        return "".join(r)

    echo = rx


class Colorize(Transform):
    """Apply different colors for received and echo"""

    def __init__(self):
        # XXX make it configurable, use colorama?
        self.input_color = "\033[36m"
        self.echo_color = "\033[31m"

    def rx(self, text):
        return self.input_color + text

    def echo(self, text):
        return self.echo_color + text


class DebugIO(Transform):
    """Print what is sent and received"""

    def rx(self, text):
        sys.stderr.write(" [RX:{}] ".format(repr(text)))
        sys.stderr.flush()
        return text

    def tx(self, text):
        sys.stderr.write(" [TX:{}] ".format(repr(text)))
        sys.stderr.flush()
        return text


# other ideas:
# - add date/time for each newline
# - insert newline after: a) timeout b) packet end character

EOL_TRANSFORMATIONS = {
    "crlf": CRLF,
    "cr": CR,
    "lf": LF,
}

TRANSFORMATIONS = {
    "direct": Transform,  # no transformation
    "default": NoTerminal,
    "nocontrol": NoControls,
    "printable": Printable,
    "colorize": Colorize,
    "debug": DebugIO,
}


class Miniterm(object):
    """
    Terminal application. Copy data from serial port to console and vice versa.
    Handle special keys from the console to show menu etc.
    """

    def __init__(self, serial_instance, echo=False, eol="crlf", filters=()):
        self.console = Console()
        self.serial = serial_instance
        self.echo = echo
        self.input_encoding = "UTF-8"
        self.output_encoding = "UTF-8"
        self.eol = eol
        self.filters = filters
        self.update_transformations()
        self.alive = None
        self._reader_alive = None
        self.receiver_thread = None
        self.rx_decoder = None
        self.tx_decoder = None
        self.pause_tic = True

    def _start_reader(self):
        """Start reader thread"""
        self._reader_alive = True
        # start serial->console thread
        self.receiver_thread = threading.Thread(target=self.reader, name="rx")
        self.receiver_thread.daemon = True
        self.receiver_thread.start()

    def _stop_reader(self):
        """Stop reader thread only, wait for clean exit of thread"""
        self._reader_alive = False
        if hasattr(self.serial, "cancel_read"):
            self.serial.cancel_read()
        self.receiver_thread.join()

    def start(self):
        """start worker threads"""
        self.alive = True
        self._start_reader()

        self.tic_thread = Periodic(3.5, self.send_tic_periodic)
        self.tic_thread.daemon = True
        self.tic_thread.start()

        # enter console->serial loop
        self.transmitter_thread = threading.Thread(target=self.writer, name="tx")
        self.transmitter_thread.daemon = True
        self.transmitter_thread.start()
        self.console.setup()

    def stop(self):
        """set flag to stop worker threads"""
        self.alive = False

    def join(self, transmit_only=False):
        """wait for worker threads to terminate"""
        self.transmitter_thread.join()
        self.tic_thread.stop()
        self.tic_thread.join()
        if not transmit_only:
            if hasattr(self.serial, "cancel_read"):
                self.serial.cancel_read()
            self.receiver_thread.join()

    def close(self):
        self.serial.close()

    def update_transformations(self):
        """take list of transformation classes and instantiate them for rx and tx"""
        transformations = [EOL_TRANSFORMATIONS[self.eol]] + [TRANSFORMATIONS[f] for f in self.filters]
        self.tx_transformations = [t() for t in transformations]
        self.rx_transformations = list(reversed(self.tx_transformations))

    def set_rx_encoding(self, encoding, errors="replace"):
        """set encoding for received data"""
        self.input_encoding = encoding
        self.rx_decoder = codecs.getincrementaldecoder(encoding)(errors)

    def set_tx_encoding(self, encoding, errors="replace"):
        """set encoding for transmitted data"""
        self.output_encoding = encoding
        self.tx_encoder = codecs.getincrementalencoder(encoding)(errors)

    def reader(self):
        """loop and copy serial->console"""
        try:
            while self.alive and self._reader_alive:
                # read all that is there or wait for one byte
                data = self.serial.read(self.serial.in_waiting or 1)
                if data:
                    text = self.rx_decoder.decode(data)
                    for transformation in self.rx_transformations:
                        text = transformation.rx(text)
                    self.console.write(text)
        except serial.SerialException:
            self.alive = False
            self.console.cancel()
            raise  # XXX handle instead of re-raise?

    def writer(self):
        """\
        Loop and copy console->serial until self.exit_character character is
        found. When self.menu_character is found, interpret the next key
        locally.
        """
        try:
            while self.alive:
                try:
                    c = self.console.getkey()
                except KeyboardInterrupt:
                    c = "\x03"
                if not self.alive:
                    break

                elif c == chr(0x14):  # Ctrl-T
                    self.send_tic()

                elif c == chr(0x19):  # Ctrl-Y
                    self.pause_tic = not self.pause_tic

                elif c == chr(0x10):  # Ctrl-P
                    global heures_pleines
                    heures_pleines = not heures_pleines

                elif c == chr(0x03):  # Ctrl-C
                    self.stop()  # exit app
                    break

                else:
                    if ord(c) < 32:
                        print("\033[36;2m<%02x>\033[0m" % ord(c))
                    text = c
                    for transformation in self.tx_transformations:
                        text = transformation.tx(text)
                    self.serial.write(self.tx_encoder.encode(text))
                    if self.echo:
                        echo_text = c
                        for transformation in self.tx_transformations:
                            echo_text = transformation.echo(echo_text)
                        self.console.write(echo_text)
        except Exception:
            self.alive = False
            raise

    def send_tic_periodic(self):
        if not self.pause_tic:
            self.send_tic()

    def send_tic(self):
        time = datetime.datetime.now().strftime("%H:%M:%S.%f")
        echo_text = f"{time} frame TIC\n"
        for transformation in self.tx_transformations:
            echo_text = transformation.echo(echo_text)
        self.console.write(echo_text)

        self.serial.write(tic())


# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# default args can be used to override when calling main() from an other script
# e.g to create a miniterm-my-device.py
def main():
    """Command line tool, entry point"""

    port = ""
    if len(sys.argv) >= 2:
        port = sys.argv[1]

    if port == "" or not os.path.exists(port):
        port = "/dev/tty.usbserial-1410"

    if not os.path.exists(port):
        port = "/dev/tty.usbserial-1420"

    try:
        serial_instance = serial.serial_for_url(port, 115200, do_not_open=True)

        if not hasattr(serial_instance, "cancel_read"):
            # enable timeout for alive flag polling if cancel_read is not available
            serial_instance.timeout = 1

        serial_instance.open()
    except serial.SerialException as e:
        sys.stderr.write("could not open port {}: {}\n".format(port, e))
        sys.exit(1)

    miniterm = Miniterm(serial_instance, echo=False, eol="crlf", filters=["colorize"])
    miniterm.set_rx_encoding("ascii")
    miniterm.set_tx_encoding("ascii")

    miniterm.start()
    try:
        miniterm.join(True)
    except KeyboardInterrupt:
        pass
    miniterm.join()
    miniterm.close()


# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
if __name__ == "__main__":
    main()
