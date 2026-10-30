.DEFAULT_GOAL := help

.PHONY: help build test test-engine test-backend clean

help:
	@echo "CampusCompute targets:"
	@echo "  make build        Build all C modules and the integrated CLI"
	@echo "  make test         Run C engine and backend API tests"
	@echo "  make test-engine  Run all standalone C and integrated CLI tests"
	@echo "  make test-backend Run FastAPI adapter tests (install backend/requirements.txt first)"
	@echo "  make clean        Remove generated C binaries and test artifacts"

build:
	$(MAKE) -C engine all

test: test-engine test-backend

test-engine:
	$(MAKE) -C engine clean test

test-backend:
	python3 -m pytest -q backend/tests

clean:
	$(MAKE) -C engine clean
	rm -rf backend/.pytest_cache backend/app/__pycache__ backend/tests/__pycache__
