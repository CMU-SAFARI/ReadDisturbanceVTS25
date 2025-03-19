import time
from tqdm import tqdm

def sleep_with_progress(seconds):
  steps = 100
  step_duration = seconds / steps
  with tqdm(total=steps, 
            desc="Sleeping", 
            bar_format="{l_bar}{bar}| {n_fmt}/{total_fmt} [Time left: {remaining}]",
            unit="%" ) as pbar:
      
      for _ in range(steps):
          time.sleep(step_duration)
          pbar.update(1)