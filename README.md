![image](https://github.com/user-attachments/assets/f70242b7-02b6-4d76-9b82-5699eb50971b)
# 과제 2 - Q3: 안티 앨리어싱 (Anti-aliasing)

 과제 목표
픽셀 경계에서 발생하는 **계단 현상(jaggies)**을 제거하고,  
부드러운 렌더링 결과를 얻기 위해 **슈퍼샘플링 기반의 안티앨리어싱**을 구현합니다.



구현 방식: Supersampling (초과 샘플링)

 핵심 아이디어
1픽셀에 대해 단 1개의 ray만 쏘는 대신,  
**64개의 무작위 ray**를 쏘고, **색상을 평균**내면  
경계선이 부드럽고 계단 현상이 줄어듭니다.

 적용 내용
- 각 픽셀에 대해 64개의 랜덤 ray를 발사
  ```cpp
  float dx = random(0, 1), dy = random(0, 1);
  Ray ray = camera->generateRay(i + dx, j + dy, width, height);
