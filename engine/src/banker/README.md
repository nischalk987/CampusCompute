# FR5 — Banker's Algorithm

This C module derives `Need = Maximum - Allocation` and runs the Banker's safety algorithm against the Available vector.

## Limits and validation
- 1–64 processes and 1–32 resource types.
- Resource counts are unsigned 32-bit integers.
- Allocation may not exceed Maximum for any process/resource pair.
- Input matrix dimensions are explicit and validated before analysis.

## Safety algorithm
Copy Available into Work, mark all processes unfinished, then repeatedly find an unfinished process whose Need is component-wise <= Work. Treat it as able to finish, add its Allocation back to Work, and append it to the sequence. If all processes can finish, the state is safe; if a full pass makes no progress, the state is unsafe.

## Build and test
From the repository root on Linux/Ubuntu or WSL Ubuntu:
```bash
make -C engine/src/banker clean all
make -C engine/src/banker test
./engine/src/banker/banker
```

The tests include the classic safe example (sequence P1 → P3 → P4 → P0 → P2), an unsafe state, invalid Allocation > Maximum, and invalid dimensions. The CLI prints Available, Allocation, Maximum, derived Need, and the computed result.

## Resource Allocation Graph (RAG)
After each safety analysis, the CLI prints a directed edge list:
- `Rk --allocated n--> Pj` represents instances currently allocated from resource Rk to process Pj.
- `Pj --remaining need n--> Rk` represents the process's remaining claim (Need = Maximum - Allocation), not necessarily a currently blocked request.

The dashboard renders the same edge directions and quantities as a visual graph.
