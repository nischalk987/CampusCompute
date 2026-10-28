from fastapi.testclient import TestClient

from app.main import app

client = TestClient(app)


def test_health_does_not_claim_unimplemented_modules_are_ready():
    response = client.get("/health")
    assert response.status_code == 200
    body = response.json()
    assert body["status"] == "ok"
    assert body["engine_modules"]["shared_memory"] == "not-implemented"
    assert body["engine_modules"]["integrated_cli"] == "not-implemented"


def test_module_capabilities_are_explicit():
    response = client.get("/api/modules")
    assert response.status_code == 200
    modules = {module["id"]: module for module in response.json()["modules"]}
    assert modules["producer"]["api"] is True
    assert modules["jobs"]["api"] is False
    assert modules["scheduling"]["api"] is False
    assert modules["banker"]["api"] is False
    assert modules["ipc"]["status"] == "not-implemented"


def test_producer_rejects_count_out_of_range():
    assert client.post("/api/producer-consumer?event_count=0").status_code == 422
    assert client.post("/api/producer-consumer?event_count=100001").status_code == 422


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
