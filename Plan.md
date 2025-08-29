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
## 16. 테스트 전략 참조 (Testing Strategy Reference)
자세한 테스트/검증 전략은 `Test.md` 문서를 참조. 각 Phase 완료 시 `Test.md`의 Exit Criteria 체크 후 이 로그(섹션 15)에 결과 반영.

---
## 17. 스타일 메트릭 공식 사양 (Style Metrics Specification)
Hypermodern/Closed 성향 측정 메트릭의 공식 정의 및 정규화 파이프라인. Phase 0은 경량 근사, Phase 1+ 정밀화.

### 17.1 Ply & 윈도우
ply_0 = 초기 FEN, ply_k = k번째 half-move 적용 후. 윈도우 평균은 실제 관측 n으로 나눔.

### 17.2 메트릭 개요
| Metric | 의미 | Window / 이벤트 | 방향 | Phase 0 | Phase 1+ |
|--------|------|-----------------|------|---------|----------|
| CO | e4/d4(백)+e5/d5(흑) 점유 | ply 1–12 평균 | 낮을수록 | 비트보드 카운트 | 동일 |
| DCC | 장거리 중앙 공격 | ply 8–16 평균 | 높을수록 | 맨해튼 근사 | 정확 attack map |
| FCH | 피안케토 구조 수 | ≤14 달성 | 높을수록 | Binary (bishop+pawn 1-step) | full/partial 구분 |
| PB_delay | 중앙 pawn tension 지연 | 최초 이벤트 (censor) | 늦을수록 | 파일/대각 검사 | en passant/공격맵 |
| CL | frontal locked pairs | ply 10–30 평균 | 높을수록 | rank3–6 frontal | central weight 옵션 |
| SI | 공간 우위 차 | ply 10–30 평균 | 높을수록 | 유니크 + pawn 0.3 | decorrelation (DCC) |

### 17.3 공식
CO = (1/n) Σ_{t∈[1,12]} (#Wpawns(e4,d4)+#Bpawns(e5,d5))_t.
DCC_t = Σ_p w(p)*|A_p|, A_p = p가 공격하는 {d4,e4,d5,e5}, w={N:1.0,B:1.1,R:1.2,Q:1.4}. DCC = 평균_{8..16}.
FCH_Phase0: bishop ∈ {g2,b2,g7,b7} ∧ file pawn 한 칸 전진(g3,b3,g6,b6). Chess960 -> 0 (flag 기록).
PB_delay: tension (frontal/diagonal/en passant) 최초 ply; 없음 → ply_end+1,censored.
CL_t: rank3–6 frontal 대치 수. CL=평균_{10..30} (샘플<6 skip flag).
SI_t: (White control(rank≥5) – Black control(rank≤4)) with pawn weight 0.3. SI=평균_{10..30}.

### 17.4 정규화 파이프라인
1) Pre-transform: PB_delay→ln(v+1); SI clip to [p05,p95]; others identity.
2) Robust scale: (x-median)/(IQR+ε), IQR<1e-6→std*0.7413 또는 range/6 fallback; clip [-3,3].
3) Map: u=(scaled+3)/6.
4) 방향: CO=1-u_CO; 나머지 유지. scaled_SI>2.5→2.5.
5) Style Score: S=Σ w_i * metric_i_norm.

### 17.5 가중치 (weights v1)
{ CO:0.20, DCC:0.25, FCH:0.10, PB_delay:0.20, CL:0.15, SI:0.10 }.

### 17.6 Censoring & JSON
PB_delay: {"value": ply_end+1, "censored": true, "reason":"no_break"}. 각 metric: {value, window:[a,b], n} 저장.

### 17.7 Drift
최근 1000 games rolling S_mean z-score>2.5 경고, >3.0 재baseline 고려.

### 17.8 Phase 차등화
Phase 1+: 공격맵 정밀화, FCH level, SI decorrelation, PB_delay survival 분석, adaptive weight (Bayesian optimization) 옵션.

### 17.9 실패 처리
IQR=0 → scale=1 fallback. 결측 metric → 0.5 + warning. censored PB_delay 학습 시 weight↓.

### 17.10 버전 관리
normalization_version (baseline 재산출 시 증가), weights_version (가중치 변경 시). JSON에 동기화.

(End of Plan)

---
## 18. 빌드 환경 & 컴파일러 정책 (Build Environment & Compiler Policy)
목표: 서버(리눅스) 및 로컬 개발 환경 재현성 확보, 성능/최적화 일관성 유지.

### 18.1 표준 컴파일러
- Primary Production: GCC 13 (Linux / WSL Ubuntu)
- Optional Validation: Clang (최적화/경고 교차 검증), MSVC (Windows 디버깅 보조)
- Deprecated / Unsupported: GCC < 11 (C++20 미충족), 구 MinGW GCC 6.x

### 18.2 WSL 표준 설정 절차
```
sudo apt update
sudo apt install -y software-properties-common
sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
sudo apt update
sudo apt install -y build-essential git python3-pip ninja-build pkg-config g++-13
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 100
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 100
python3 -m pip install --user meson
```

### 18.3 기본 빌드 흐름
```
~/.local/bin/meson setup builddir -Dnative_arch=true
~/.local/bin/meson compile -C builddir
~/.local/bin/meson test -C builddir StyleMetricsTest --print-errorlogs
```
Release 빌드:
```
~/.local/bin/meson configure builddir -Dbuildtype=release -Dnative_arch=true
~/.local/bin/meson compile -C builddir
```

### 18.4 성능 / 재현성 정책
- 개발 중 빠른 피드백: `-Dbuildtype=debugoptimized`.
- 퍼포먼스 벤치/Elo: `release + native_arch=true`.
- 재현 가능한 배포(아키텍처 중립): native_arch=false + 명시 `-march=x86-64-v3` (추가 옵션 TBD) → 별도 builddir.
- LTO: Meson 기본 옵션 유지 (`b_lto=true`). 필요 시 profile-guided 최적화(PGO) Phase 3+ 고려.

### 18.5 GPU / Backend 호환성 노트
| Backend | 체크 항목 | 주석 |
|---------|----------|------|
| CUDA | Toolkit 버전 ↔ 허용 GCC 범위 | CUDA 12.x: GCC ≤13 권장. 버전 고정 시 문서화 |
| OpenCL | ICD & 헤더 경로 | GCC/Clang 차 최소 |
| SYCL (oneAPI) | DPC++(Clang) 사용 | 코어 엔진은 GCC로, SYCL만 별도 toolchain 가능 |
| BLAS / oneDNN | 라이브러리 링크 호환 | -DUSE_BLAS 플래그 영향 성능 측정 |

### 18.6 CI 매트릭스 (향후)
- gcc13 Debug / Release
- clang 최신 Release (경고/UB 체크)
- 옵션: style_metrics normalization drift 체크 job

### 18.7 문제 대응 가이드
| 증상 | 원인 | 해결 |
|------|------|------|
| Meson setup 중 C++20 에러 | 구버전 GCC | g++-13 설치 후 update-alternatives |
| Subproject (abseil) 실패 | 표준 플래그 미지원 | 컴파일러 교체 (gcc13/clang) |
| Link OpenCL 실패 | 헤더/ICD 누락 | distro 패키지(opencl-headers, ocl-icd-libopencl1) 설치 |
| CUDA 컴파일 오류 | GCC 버전 과다 신형 | CUDA release notes 확인, 하위 GCC 설치 |

### 18.8 버전 고정 정책
- 기록: `docs/build_env.json` (추후) → { gcc_version, cuda_version, normalization_version, weights_version }.
- 변경 시 체인지로그(Changelog) & Plan.md §18 업데이트.

### 18.9 향후 개선
- ccache / sccache 통합
- PGO 프로파일 자동 수집 (self-play run tag)
- Sanitizer 빌드 (clang ASan/UBSan) 별도 파이프라인

(End §18)
