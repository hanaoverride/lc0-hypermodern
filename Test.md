# 테스트 전략 (Testing Strategy) – LC0 Hypermodern Fork

(한국어 우선 / English key terms 병기)

---
## 1. 목적 (Purpose)
이 문서는 Hypermodern & Closed 스타일 편향 기능을 단계적으로 도입하는 과정에서 **정확성( correctness )**, **성능(performance)**, **스타일 목표 달성(style adherence)**, **회귀 방지(regression prevention)**, **재현성(reproducibility)**을 보장하기 위한 테스트 프레임워크를 정의한다.

---
## 2. 범위 (Scope)
포함:
- 코드 변경: encoder 확장, search prior bias, self-play opening injection, Style Head, UCI 옵션.
- 파이프라인: self-play data generation, metrics 계산, 훈련/평가.
- 산출물: 네트워크 파일, 스타일 지표 리포트, Elo 테스트 로그.

제외:
- 외부 인프라(클러스터 예약 시스템), GUI 통합.

---
## 3. 용어 (Glossary)
- Style Score (S): Plan.md 정의된 가중합.
- Baseline Network: 변경 전 동일 구조의 기준 모델.
- SPRT: Sequential Probability Ratio Test.
- Throughput: positions/s or eval/s.
- Bias Level: StyleBias UCI 옵션 값.

---
## 4. 테스트 분류 (Test Categories)
1. 단위 테스트 (Unit Tests): 비트보드 / feature 계산 / heuristic 함수 → O(밀리초)
2. 구성 테스트 (Configuration): UCI 옵션 등록/파싱, StyleBias 값 변경 적용 여부.
3. 기능 테스트 (Functional): Search prior scaling, opening injection 효과 논리 검증.
4. 통합 테스트 (Integration): self-play loop + metrics 파이프라인 end-to-end.
5. 회귀 테스트 (Regression): 기존 엔진 출력(Elo, PV 안정성) 비교.
6. 성능 테스트 (Performance): throughput, latency, GPU 메모리 이용률.
7. 통계적 평가 (Statistical): Style Score 분포 shift, Elo SPRT.
8. 신뢰성/재현성 (Determinism): seed 고정 시 동일 결과 확인.

---
## 5. 공통 메트릭 (Common Metrics)
| Metric | 목적 | 기준 (Baseline 이후) |
|--------|------|----------------------|
| S_mean_shift | 스타일 달성 | ≥ +1.0σ (Phase 3) |
| Elo_diff | 체스 실력 | ≥ 0 (Phase 3+), 손실 허용 ≤20 Elo (Phase 1) |
| Throughput_delta | 성능 유지 | |Δ| ≤5% (Phase 2+) |
| GPU Mem Δ | 자원 | ≤ +10% (feature 추가 후) |
| Crash rate | 안정성 | 0 in N games |
| Determinism hash | 재현 | seed 고정 시 identical |

---
## 6. Phase별 테스트 전략 (Per-Phase Strategy)
### Phase 0 – Baseline Audit
목표: 스타일 지표 계산기 신뢰성 검증.
- 단위: central occupation, locked pawn, attack map 함수 → 사전 정의된 FEN 입력 vs expected.
- 교차 검증: 10개 랜덤 게임 PGN → 수동 계산 spot-check.
- 회귀 스냅샷: baseline Style Score 분포 (JSON 저장 `baseline_metrics.json`).
Exit Criteria:
- 모든 unit test PASS.
- Baseline S 분포 JSON 저장 & 커밋.

### Phase 1 – Search Soft Bias + Opening Injection
목표: 지표 shift 관찰, Elo 손실 제한.
- 기능: opening seed 적용률 (로그에서 95% 이상 원하는 세트 내 시작).
- 단위: heuristic B(m) 각 구성요소 ΔDCC, fianchetto 감지 테스트.
- 통합: 1k self-play (Bias=Off/Medium) → S_mean 비교.
- 성능: throughput 변화 측정 (±2% 이내 기대).
- 통계: small sample t-test 또는 bootstrap CI for S_mean.
Exit Criteria:
- Bias=Off vs Medium: S_mean 차이 p<0.01.
- Elo 손실 ≤20 (2k game quick match, SPRT inconclusive or pass neutral).

### Phase 2 – Feature Planes + Style Head
목표: 모델이 스타일을 내재 학습.
- 단위: encoder plane output shape & value sanity (locked pawn plane bitcount).  
- 구성: proto 확장 backward compatibility (구버전 net 로드 실패 대비 graceful error).
- 학습: mini-train 5k batches → style head loss 감소 곡선 기록.
- A/B: Bias Off 상태에서 S_mean 여전히 +0.5σ 이상 (네트워크 자체 표현).
- 성능: GPU 메모리 & eval/s 측정 (≤5% 저하).
Exit Criteria:
- Style head validation MSE < 0.02 (normalized scale) or monotonic 감소.
- Throughput_delta ≤5%.

### Phase 3 – Curriculum & Tuning
목표: Bias 감소 중 성능/스타일 유지.
- 실험 매트릭스: (λ_s ∈ {0.1,0.2,0.3}, β_schedule ∈ {Linear, Exponential}) → 각 1k games 샘플.
- 분석: Pareto frontier (Elo, S_mean).
- 통계: ANOVA or Kruskal-Wallis on S_mean across configs.
Exit Criteria:
- 선택 config: S_mean ≥ +1.0σ, Elo diff ≥ 0.

### Phase 4 – Ablation & Refinement
목표: 불필요한 피처 제거.
- Ablation: 각 feature plane 제거 후 500 games S_mean & throughput.
- 중요도 ranking: ΔS_mean / ΔThroughput trade-off.
Exit Criteria:
- 제거 리스트 확정, Off 모드 Elo ≈ baseline (±5 Elo).

### Phase 5 – Packaging & Release
- 회귀: 최종 회귀 테스트 스위트 (전체 Unit + 기능 + determinism).
- 문서 검증: README 스타일 사용 예 QA.
- 최종 SPRT vs baseline (strong settings) PASS or Neutral.
Exit Criteria:
- 모든 테스트 PASS, DoD 충족.

---
## 7. 단위 테스트 세부 (Unit Test Details)
우선 순위 함수:
- `ComputeCentralOccupation(position)`
- `ComputeLockedPawns(position)`
- `ComputeDistantCentralControl(position)`
- `ComputeFianchettoStatus(position)`
- Heuristic `ComputeMoveStyleDelta(position, move)`
테스트 패턴: 최소 1 포지션 = 1 assert set, 캡처된 기대값 JSON 보관.

---
## 8. 기능/통합 테스트 (Functional/Integration)
시나리오:
1. Opening Injection: seed 목록 중 하나로 100% 시작 여부.
2. StyleBias Level 변화 → root priors log 비교 (Medium > Off for style-consistent moves).
3. Self-play 50 games smoke: no crashes, metrics pipeline 완료.
로깅 확장: `--log_style` 옵션 (추가) → per-move B(m) 기록.

---
## 9. 성능 테스트 (Performance)
방법:
- 동일 하드웨어, 고정 network, 10k node search 반복 30회 측정 → 평균 & 95% CI.
- Feature 추가 전/후 diff.
- 프로파일: (옵션) perf or nvprof-like (플랫폼 한정) – 문서화 only.

---
## 10. 통계 방법 (Statistical Methods)
- Style Score shift: bootstrap (10k resamples) 95% CI.
- Elo: SPRT (H0: μ≤0, H1: μ≥25) → boundaries (A=−2.94, B=2.94 예시; 조정 가능).
- Feature importance: Ablation ΔS_mean z-score.

---
## 11. 자동화 파이프라인 (Automation Pipeline)
CI (예: GitHub Actions / local script):
1. Build & Unit tests
2. 20 game smoke self-play (Bias Off/Medium)
3. Metrics extraction → artifact upload
4. Performance micro-benchmark (optional nightly)
5. On demand: longer Elo match triggered by tag `elo-test`.

아티팩트:
- `baseline_metrics.json`, `current_metrics.json`
- `throughput_report.txt`
- `elo_progress.csv`

---
## 12. 데이터 보존 & 재현 (Data Retention & Reproducibility)
- 각 Phase 결과 디렉토리: `experiments/phaseX/runY/` (config.json, seed, commit hash, metrics.json, network.pb.gz).
- 해시: Style feature plane 버전 번호 (e.g., `STYLE_FEAT_VER=1`).

---
## 13. 리스크 & 완화 (Testing Risks)
| 리스크 | 설명 | 완화 |
|--------|------|------|
| 무작위성으로 인한 S_mean 변동 | 샘플 부족 | Bootstrap + 충분한 게임 수 | 
| Elo 테스트 시간 과다 | 긴 매치 지연 | Rapid time control + SPRT 조기 종료 |
| 성능 회귀 미탐지 | 미세 저하 누적 | 주간 성능 벤치 강제 수행 |
| Feature 계산 오류 은닉 | 복합 파생지표 | 단위 테스트 세분화 + spot-check |

---
## 14. Exit Criteria 요약
| Phase | Style | Elo | 성능 | 기타 |
|-------|-------|-----|------|------|
| 0 | Baseline 분포 확보 | n/a | n/a | Unit PASS |
| 1 | +0.5σ | ≥ -20 | ≤2% 저하 | Bias 효과 유의 |
| 2 | +0.8σ | 회복 | ≤5% 저하 | Style head 학습 |
| 3 | +1.0σ | ≥ 0 | ≤5% 저하 | Schedule 안정 |
| 4 | 유지 | ≥ 0 | 최적화 | Ablation 완료 |
| 5 | 목표 유지 | ≥ 0 | 유지 | DoD 충족 |

---
## 15. 차후 확장 (Future Testing Enhancements)
- Property-based tests (랜덤 FEN → invariants).
- Differential testing vs vanilla 엔진 (동일 seed search trace 비교).
- Style adversarial positions: engine drift 탐지.

---
## 16. 빠른 실행 예 (Quick Commands – Outline)
(실제 스크립트 구현 후 문서 업데이트)
- Smoke self-play 오프라인
- Metrics 추출 & 비교

---
## 17. 업데이트 규칙 (Maintenance)
기능 추가시 Test.md 내 관련 Phase/Exit Criteria 수정 → PR template 체크박스 연계 권장.

---
(End of Test Strategy)
