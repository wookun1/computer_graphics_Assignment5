![image](https://github.com/user-attachments/assets/9b3144a0-5e30-4ab9-a645-8dde9593ba5a)

# 과제 5 – Q1: 소프트웨어 래스터라이저 + Z-버퍼

## 과제 목표  
제공된 단위 구(unit‐sphere) 메시를 변환 및 투영하여,  
소프트웨어 래스터라이저와 깊이(Z) 버퍼를 이용해 비하이라이트(언셰이디드) 구를 렌더링합니다. :contentReference[oaicite:0]{index=0}:contentReference[oaicite:1]{index=1}

---

## 처리 순서

1. **메시 생성**  
   - `create_sphere(width=32, height=16)` 함수 내에서  
     - 중간 고리 정점 생성: 위도 `θ` ∈ (0, π), 경도 `φ` ∈ [0, 2π)  
     - 상·하 극점 추가  
     - 측면(side) 삼각형 + 상단(top cap)·하단(bottom cap) 삼각형 인덱스 구축  

2. **변환 파이프라인**  
   ```cpp
   mat4 Model      = translate(mat4(1.0f), vec3(0,0,-7))
                    * scale    (mat4(1.0f), vec3(2.0f));    // 반경 2, 위치 (0,0,-7)
   mat4 View       = lookAt  (vec3(0,0,0), vec3(0,0,-1), vec3(0,1,0));  // 카메라 at (0,0,0)
   mat4 Projection = frustum (-0.1f,0.1f, -0.1f,0.1f, 0.1f, 1000.0f);   // 원근 투영
   mat4 MVP        = Projection * View * Model;


소프트웨어 래스터라이저 + Z-버퍼

버퍼 초기화
FrameBuffer.assign(512*512*3, 0.5f);                   // 회색 배경
ZBuffer.assign   (512*512,   +∞);                      // 무한대로 초기화

삼각형 순회 & 래스터
for(int t=0; t<gNumTriangles; ++t) {
  // 정점 3개 → 화면 좌표 P0,P1,P2
  float area = edge(P0,P1,P2);
  if(|area|<ε) continue;                               // 평평한 삼각형 스킵

  // 바운딩 박스
  for(y=minY; y<=maxY; ++y)
    for(x=minX; x<=maxX; ++x) {
      // 람버트 면적 좌표 계산
      if(w0>=0 && w1>=0 && w2>=0) {
        z = w0*P0.z + w1*P1.z + w2*P2.z;
        if(z < ZBuffer[y*512 + x]) {
          ZBuffer[y*512 + x] = z;
          FrameBuffer[(y*512+x)*3 + ...] = vec3(1.0f); // 흰색
        }
      }
    }
}


결과 
512×512 해상도의 회색 배경 위에,

깊이 버퍼 기반으로 올바르게 가려진 흰색 구가 렌더됨
