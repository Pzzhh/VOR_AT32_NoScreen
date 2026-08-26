def test_websocket_runtime_is_installed() -> None:
    import websockets

    assert websockets.__version__
