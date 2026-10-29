from fastapi.testclient import TestClient

from backend.app.main import app

client = TestClient(app)
LOCAL_ORIGIN = {"Origin": "http://127.0.0.1:5173"}


def test_terminal_pwd_executes_real_command():
    response = client.post(
        "/api/terminal/execute",
        headers=LOCAL_ORIGIN,
        json={"command": "pwd", "cwd": "."},
    )
    assert response.status_code == 200
    data = response.json()
    assert data["exit_code"] == 0
    assert data["cwd"] == "."
    assert data["stdout"].strip()


def test_terminal_cd_updates_workspace_directory():
    response = client.post(
        "/api/terminal/execute",
        headers=LOCAL_ORIGIN,
        json={"command": "cd frontend", "cwd": "."},
    )
    assert response.status_code == 200
    assert response.json()["cwd"] == "frontend"


def test_terminal_rejects_shell_chaining():
    response = client.post(
        "/api/terminal/execute",
        headers=LOCAL_ORIGIN,
        json={"command": "echo hello && whoami", "cwd": "."},
    )
    assert response.status_code == 400


def test_terminal_rejects_unknown_commands():
    response = client.post(
        "/api/terminal/execute",
        headers=LOCAL_ORIGIN,
        json={"command": "sh -c whoami", "cwd": "."},
    )
    assert response.status_code == 400


def test_terminal_rejects_untrusted_origin():
    response = client.post(
        "/api/terminal/execute",
        headers={"Origin": "https://example.com"},
        json={"command": "pwd", "cwd": "."},
    )
    assert response.status_code == 403


def test_terminal_rejects_parent_directory():
    response = client.post(
        "/api/terminal/execute",
        headers=LOCAL_ORIGIN,
        json={"command": "pwd", "cwd": "../"},
    )
    assert response.status_code == 400


def test_terminal_project_command_reports_project_details():
    response = client.post(
        "/api/terminal/execute", headers=LOCAL_ORIGIN,
        json={"command": "project", "cwd": "."},
    )
    assert response.status_code == 200
    data = response.json()
    assert data["exit_code"] == 0
    assert "CampusCompute" in data["stdout"]
    assert "CPU scheduling" in data["stdout"]


def test_terminal_modules_command_reports_module_status():
    response = client.post(
        "/api/terminal/execute", headers=LOCAL_ORIGIN,
        json={"command": "modules", "cwd": "."},
    )
    assert response.status_code == 200
    assert "producer" in response.json()["stdout"]
    assert "banker" in response.json()["stdout"]


def test_terminal_ls_is_platform_independent():
    response = client.post(
        "/api/terminal/execute", headers=LOCAL_ORIGIN,
        json={"command": "ls", "cwd": "."},
    )
    assert response.status_code == 200
    assert response.json()["exit_code"] == 0
    assert "frontend/" in response.json()["stdout"]


def test_terminal_help_lists_project_commands():
    response = client.post(
        "/api/terminal/execute", headers=LOCAL_ORIGIN,
        json={"command": "help", "cwd": "."},
    )
    assert response.status_code == 200
    assert "build [scheduler|producer]" in response.json()["stdout"]
