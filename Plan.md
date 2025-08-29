# Hypermodern & Closed-Position Preference Plan (LC0 Hypermodern Fork)

(한국어 우선, 중요 용어 병기 / English key terms in parentheses)

---
## 1. 비전 & 스코프 (Vision & Scope)
이 포크는 기본 LCZero 체스 실력을 크게 훼손하지 않으면서, 다음 두 가지 스타일적 성향을 **명시적으로 강화**하는 것을 목표로 한다.
1. Hypermodern 오프닝 지향: 전통적 즉시 중심 점유(1.e4, 1.d4 → e4/d4/e5/d5 조기 점유)를 늦추고, **원거리 피스 컨트롤로 중앙을 간접 제어** (fianchetto, c-pawn/d-pawn 지연, ...).
2. Closed / Semi-Closed 전투 선호: 조기 중앙 파열(pawn break) 회피, pawn chain 유지, maneuvering space 확보.

최종 산출물:
- (a) 스타일 스코어(style adherence score)가 높은 self-play 데이터셋
- (b) 스타일 인식/조절이 가능한 신경망 (추가 head 또는 내재 표현)
- (c) UCI 옵션: `StyleBias` (0=기본, 1=온건, 2=강함)
- (d) 재현 가능한 학습 & 평가 파이프라인

---
## 2. 스타일 정의 (Formal Style Definition)
Hypermodern / Closed 성향을 **측정 가능 지표**로 수치화.

| 카테고리 | 지표 (Metric) | 설명 |
|----------|---------------|------|
| 중앙 점유 (Central Occupation) | CO_t = (# white pawns on e4/d4 + black on e5/d5 at ply t) | 초반(≤12 ply) 낮을수록 hypermodern |
| 원거리 제어 (Distant Central Control) | DCC_t = sum(piece attacks into {d4,e4,d5,e5} / piece value norm) | 말/비숍/퀸 장거리 공세 비중 |
| Fianchetto 활용 | FCH = count of g2/b2/g7/b7 bishop fianchetto achieved by ply 14 | 2 이상이면 강한 경향 |
| Pawn Break 지연 | PB_delay = first ply where c/d/e pawn advances create tension (contact) | 지연 ↑ | 
| Closedness (봉쇄도) | CL_t = locked pawn pairs (# of opposing pawns directly blocked) | 평균 CL_t ↑ |
| Space Imbalance 유지 | SI = avg( own controlled squares beyond rank4 vs opp ) | 조기 급증 억제, 완만 증가 |

Style Score S = w1*(1-CO_norm) + w2*DCC_norm + w3*FCH_norm + w4*PB_delay_norm + w5*CL_norm (가중치 합=1). 가중치는 탐색적 튜닝 (초기 w = [0.2,0.25,0.15,0.2,0.2]).

---
## 3. 베이스라인 감사 (Baseline Audit)
Phase 0에서 vanilla LC0 (동일 네트워크) 10k self-play 게임 수집 → 상기 지표 분포 추출 → 목표 편차 정의.
- 타겟: S_mean + 1.0σ 향상 (Hypermodern 편향 세기 1단계)
- Regression: Elo (SPRTest vs master) -20 Elo 이하 손실 허용 (Phase 1)

---
## 4. 접근 개요 (Approach Overview)
세 축:
1. Data Shaping: 시작 포지션 다양화 + 스타일 가중 self-play 커리큘럼.
2. Network Augmentation: 추가 입력 피처 & Style Head (multi-task 학습).
3. Search Bias Layer: 정책 prior/노드 selection 조정 (온건, 가벼운 휴리스틱; 네트워크 개선 이후 축소 가능).

---
## 5. 단계별 로드맵 (Phased Roadmap)
| Phase | 목표 | 기간(상대) | 성공 기준 |
|-------|------|-----------|------------|
| 0 | Baseline 측정 & 스크립트 | Week 1 | 지표 추출 도구, S_baseline 확정 |
| 1 | Opening Injection + Search Soft Bias (네트워크 미변경) | Week 2-3 | S +0.5σ, Elo 손실 ≤20 |
| 2 | Feature Engineering + Style Head 도입, 재훈련 v1 | Week 4-6 | S +0.8σ, Elo 손실 회복 |
| 3 | Curriculum (bias schedule) + Loss Tuning | Week 7-8 | S +1.0σ, Elo ≥ baseline |
| 4 | Ablation & Refinement (feature pruning, bias 감소) | Week 9 | Bias 옵션 off 시 vanilla 동등 |
| 5 | Packaging & UCI 옵션, 문서 | Week 10 | Release candidate |

---
## 6. 기술 변화 상세 (Technical Changes)
### 6.1 데이터 & 시작 포지션
- `scripts/`에 Hypermodern Opening JSON: A00, A40, Réti, King's Indian, Grünfeld, Nimzo/Queen's Indian, Pirc/Modern, Catalan 등 FEN seed.
- Self-play loop 수정 (`selfplay/loop.cc` 또는 tournament 생성부): opening selection 확률 p(style)=StyleBiasLevel * α + base.
- Closed 목표: early symmetrical pawn tension 회피 → move filter: if creating immediate central pawn contact before ply 8, temperature↑ 또는 prior scale↓.

### 6.2 스타일 피처 (Input Feature Plan)
새 plane 후보 (encoder 확장 필요 `neural/encoder.cc`):
1. Locked pawn mask (각 색)
2. Pawn tension squares
3. Central control map (attack count clipped 0..N)
4. Fianchetto squares occupancy / potential (bishop + adjacent pawns)
5. Move-count since central pawn advance (stacked scalar planes)

메모리 영향 평가: kPlanes 증가 → GPU throughput 영향 측정 (추가 ≤ +8 planes 목표).

### 6.3 Style Head
- 네트워크 protobuf (`proto/net.pb.*`) 확장: optional `style_head` logits (regression: predicted Style Score S_hat ∈ [0,1]).
- Loss: L = L_policy + λ_v * L_value + λ_s * L_style (MSE or BCE) + regularization.
  - 초기 λ_s = 0.2 (Phase 2), 이후 grid search.
- Self-play 시 각 position에 Style Score S_tag 계산 (on-the-fly or postprocess) → training frame에 추가.

### 6.4 Search Soft Bias
- Root에서 candidate moves 평가 시: heuristic style bonus B(m) ∈ [−b,b].
  - Heuristic features: (a) increases DCC, (b) delays central pawn contact, (c) develops fianchetto, (d) maintains locked chain.
- Implementation 전략:
  - 정책 확률 p_m → p'_m ∝ p_m * exp(β * B(m)) (β: StyleBiasLevel dependent).
  - 위치 재탐색 방지 위해 node key에 bias 영향 미포함 (순수 prior scaling).
- UCI 옵션 추가: `StyleBias` (enum Off/Light/Medium/Strong) → β = {0, 0.3, 0.6, 1.0}.

### 6.5 Curriculum
- StyleBias schedule: 초반 generation strong → 점차 Medium/Light로 감소 (네트워크 자체 학습 유도).
- Replay buffer 균형: style-high / neutral 비율 60/40 목표.

### 6.6 Tooling
- Metrics extractor: 파이썬 (`scripts/gen_style_metrics.py`) → PGN/JSON ingest; outputs per-game S.
- Visualization: Jupyter optional (not required in repo core).

---
## 7. 메트릭 & 평가 (Evaluation)
1. Style Metrics: S_mean, distribution shift vs baseline (KS test p-value).
2. Elo / SPRT: vs vanilla network (same trunk size) – pass if Elo diff ≥ 0 or within error margin after Phase 3.
3. Throughput: nodes/s, eval/s 변동 ≤5% (Phase 2 이후), memory footprint < +10%.
4. Regression: no crash, determinism (seeded) stable.

---
## 8. 위험 & 완화 (Risks & Mitigations)
| 위험 | 영향 | 완화 |
|------|------|------|
| 과도한 스타일 → Elo 손실 | 채택 실패 | Curriculum + optional disable |
| Feature plane 증가로 속도 저하 | 학습/서치 지연 | Plane 수 제한, ablation |
| Style score 계산 비용 | Self-play 속도 감소 | Lazy caching + incremental update |
| Search bias와 NN gradient misalignment | convergence 늦음 | Bias decay schedule |
| FEN seed 다양성 부족 | overfit 특정 라인 | Large curated opening set |

---
## 9. 상세 작업 체크리스트 (Actionable Tasks)
### Phase 0
- [ ] Baseline self-play 10k games 수집 스크립트
- [ ] Style metric 계산기 구현 (CO, DCC, FCH, PB_delay, CL)
- [ ] Baseline 보고서 작성

### Phase 1
- [ ] Opening seed JSON 추가
- [ ] Self-play loop opening injection 코드
- [ ] Search prior scaling hook (`search/...`) 위치 식별 & 구현
- [ ] UCI 옵션 `StyleBias` 등록
- [ ] 10k games 재수집 & S_shift 평가

### Phase 2
- [ ] Style feature plane 정의 문서화
- [ ] `encoder` 확장 (추가 plane 계산)
- [ ] Proto 확장: style_head
- [ ] Loss 함수 λ_s 통합 (training pipeline 외부 레포면 sync 필요)
- [ ] Style score tagging 파이프라인 (writer 확장 or sidecar JSON)
- [ ] 재훈련 (v1) 및 벤치마크

### Phase 3
- [ ] Curriculum bias schedule 구현
- [ ] λ_s, β grid search 자동화 스크립트
- [ ] 결과 분석 (trade-off curves)

### Phase 4
- [ ] Ablation: remove each feature plane → impact 평가
- [ ] Bias Off 성능 재검증 (equivalence)

### Phase 5
- [ ] 문서화 (README 스타일 섹션)
- [ ] Release packaging (network + config)
- [ ] 예제 UCI 사용 가이드

(각 태스크 완료시 날짜 & 커밋 해시 기록 권장)

---
## 10. 설계 세부 (Design Details)
### 10.1 Style Score 실시간 계산 (Pseudo)
```
Update(position):
  update central_occup(); // pawn on e4/d4/e5/d5
  update attack_map();    // bitboards for piece attacks
  update locked_pairs();  // opposing pawn directly blocked
  ... accumulate features over early-game window (ply <= 24)
Finalize(game):
  compute per-ply metrics -> normalize -> aggregate to S
```

### 10.2 Normalization 초기값
- Min/Max or Robust Z-score (median/IQR) from baseline sample.
- Store in JSON `style_norm.json` → reproducible.

### 10.3 Heuristic Bonus B(m)
B(m) = Σ h_i(m) * α_i, where h_i:
- Fianchetto develop (bishop to g2/b2/g7/b7 with supporting pawn) → +1
- Avoids early central pawn contact (if move does not create new tension before ply 8) → +0.5
- Increases distant central attack count (ΔDCC > 0) → +0.3
- Advances side pawn preparing later break (a/h pawn minor effect) → +0.1 (tunable)
Scale: clip B in [-1.0, +1.0]; α_i to be tuned.

### 10.4 Style Head Architecture
- Input: shared trunk final embedding
- Head: Global average pool → Dense(128, Swish) → Dense(1, Sigmoid)

---
## 11. 향후 확장 (Future Ideas)
- Opponent-adaptive style (switch bias if opponent chooses anti-hypermodern setups)
- Multi-style framework (Aggressive, Endgame-centric, etc.)
- RL reward shaping instead of supervised style head.

---
## 12. Appendix: 용어 요약
- Hypermodern: 중앙을 말/비숍/퀸으로 제어, central pawn 전진 지연.
- Closed: pawn chain 형성으로 파일/대각선 차단, maneuvering 증가.
- Style Bias: 검색 priors 수정 + 네트워크 multi-task로 스타일 점수 높이기.

---
## 13. 성공 정의 (Definition of Done)
- 옵션 Off 시 원래 Elo ±5 Elo 이내.
- StyleBias=Medium 시 S_mean ≥ baseline + 1.0σ.
- Throughput 감소 ≤5%.
- 문서 & 재현 스크립트 포함.

---
## 14. 빠른 스타트 (Quick Start Outline)
1. Baseline games 생성 → metrics 추출
2. Opening seeds 추가 & search bias 적용 → Phase 1 평가
3. Feature + style head 도입 → 재훈련
4. Curriculum + 튜닝
5. Ablation & 문서화 → Release

---
## 15. 상태 추적 (Progress Log Template)
| Date | Phase | Task | Commit | Note |
|------|-------|------|--------|------|
|      |       |      |        |      |

---
(End of Plan)
