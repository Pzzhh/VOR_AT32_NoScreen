from app.protocol import Ack, Event, format_command, parse_line


def test_ack_and_event_parsing() -> None:
    ack = parse_line("Ctrl:ACK ID=7 CMD=VOR.Start CODE=0 STATE=RUNNING MSG=STARTED")
    assert isinstance(ack, Ack)
    assert ack.accepted and ack.request_id == 7

    event = parse_line("Ctrl:EVT SEQ=12 EVENT=PROGRESS MODE=VOR STATE=RUNNING CODE=0 PCT=35 REM=65")
    assert isinstance(event, Event)
    assert event.progress == 35 and event.remaining == 65


def test_command_format() -> None:
    assert format_command("Start", 3, "C=20 V=50 F=1.6 E=0") == (
        b"Ctrl:VOR.Start ID=3 C=20 V=50 F=1.6 E=0\r\n"
    )
    assert format_command("Start", 4, "A=15", "INC") == (
        b"Ctrl:INC.Start ID=4 A=15\r\n"
    )


def test_encoder_event_parsing() -> None:
    event = parse_line("Ctrl:EVT SEQ=13 EVENT=ENCODER MODE=VOR STATE=RUNNING CODE=0 PCT=35 REM=65 ANGLE=-12.345")
    assert isinstance(event, Event)
    assert event.angle == -12.345
