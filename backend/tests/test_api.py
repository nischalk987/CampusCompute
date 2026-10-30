from fastapi.testclient import TestClient
from fastapi import HTTPException
from pathlib import Path
from unittest.mock import patch
import subprocess

from app import main
from app.main import app

client = TestClient(app)


def test_health_does_not_claim_unimplemented_modules_are_ready():
    response = client.get("/health")
    assert response.status_code == 200
    body = response.json()
    assert body["status"] == "ok"
    assert body["engine_modules"]["shared_memory"] == "implemented-cli-only"
    assert body["engine_modules"]["integrated_cli"] == "implemented"


def test_module_capabilities_are_explicit():
    response = client.get("/api/modules")
    assert response.status_code == 200
    modules = {module["id"]: module for module in response.json()["modules"]}
    assert modules["producer"]["api"] is True
    assert modules["jobs"]["api"] is False
    assert modules["scheduling"]["api"] is False
    assert modules["banker"]["api"] is False
    assert modules["ipc"]["status"] == "implemented-cli-only"


def test_producer_rejects_count_out_of_range():
    assert client.post("/api/producer-consumer?event_count=0").status_code == 422
    assert client.post("/api/producer-consumer?event_count=100001").status_code == 422
    assert client.post("/api/producer-consumer?event_count=abc").status_code == 422


def test_producer_executes_real_c_module():
    response = client.post("/api/producer-consumer?event_count=3")
    assert response.status_code == 200, response.text
    body = response.json()
    assert body["source"] == "C engine"
    assert body["events_produced"] == 3
    assert [entry["sequence"] for entry in body["events"]] == [1, 2, 3]
    assert body["summary"].startswith("Summary: produced=3 consumed=3 queued=0")


def test_arbitrary_shell_endpoint_does_not_exist():
    assert client.post("/api/terminal?command=echo+hello").status_code == 404


def test_execution_timeout_is_reported_as_gateway_timeout():
    executable = Path("/tmp/fixed-engine-fixture")
    with patch.object(main, "_executable", return_value=executable), \
         patch.object(main, "_run_fixed", side_effect=HTTPException(504, "C module exceeded the execution time limit.")):
        response = client.post("/api/producer-consumer?event_count=1")
    assert response.status_code == 504
    assert "time limit" in response.json()["detail"]


def test_missing_engine_is_reported_as_unavailable():
    with patch.object(main, "_executable", side_effect=HTTPException(503, "Unable to build module: gcc missing")):
        response = client.post("/api/producer-consumer?event_count=1")
    assert response.status_code == 503
    assert "gcc missing" in response.json()["detail"]


def test_nonzero_engine_exit_is_reported_actionably():
    with patch.object(main, "_executable", return_value=Path("/tmp/fixed-engine-fixture")), \
         patch.object(main, "_run_fixed", side_effect=HTTPException(422, "logger did not finish cleanly")):
        response = client.post("/api/producer-consumer?event_count=1")
    assert response.status_code == 422
    assert "did not finish cleanly" in response.json()["detail"]


def test_malformed_engine_log_is_not_returned_as_success():
    fake_output = subprocess.CompletedProcess(
        args=["producer_consumer"], returncode=0, stdout="Summary: produced=1 consumed=1 queued=0\n", stderr=""
    )
    with patch.object(main, "_executable", return_value=Path("/tmp/fixed-engine-fixture")), \
         patch.object(main, "_run_fixed", return_value=fake_output), \
         patch.object(main.tempfile, "TemporaryDirectory") as tempdir:
        tempdir.return_value.__enter__.return_value = "/tmp/nonexistent-test-dir"
        tempdir.return_value.__exit__.return_value = False
        response = client.post("/api/producer-consumer?event_count=1")
    assert response.status_code == 502
