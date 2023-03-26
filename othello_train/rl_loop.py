import argparse
import subprocess
from pathlib import Path
from typing import Optional


def check_call(args, skip_if_exists: Optional[Path] = None):
    if skip_if_exists is not None and skip_if_exists.exists():
        print("#skip: " + " ".join(args))
        return
    print(" ".join(args))
    subprocess.check_call(args)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("work_dir")
    parser.add_argument("--model", required=True)
    parser.add_argument("--model_kwargs")
    parser.add_argument("--epoch", type=int, default=10)
    parser.add_argument("--train_epoch", type=int, default=5)
    parser.add_argument("--games", type=int, default=10000)
    parser.add_argument("--batch_size")
    parser.add_argument("--playout_limit")
    args = parser.parse_args()

    work_dir = Path(args.work_dir)
    records_dir = work_dir / "records"
    records_dir.mkdir(parents=True, exist_ok=True)

    model_args = []
    if args.model:
        model_args.extend(["--model", args.model])
    if args.model_kwargs:
        model_args.extend(["--model_kwargs", args.model_kwargs])

    for epoch in range(args.epoch):
        if epoch == 0:
            check_call(["python", "-m", "othello_train.make_empty_model_v1",
                       f"{work_dir}/cp_{epoch}/cp"] + model_args, work_dir / f"cp_{epoch}")
            check_call(["python", "-m", "othello_train.checkpoint_to_savedmodel_v1",
                        f"{work_dir}/cp_{epoch}/cp", f"{work_dir}/sm_{epoch}"] + model_args, work_dir / f"sm_{epoch}")
        playout_args = ["python", "-m", "othello_train.playout_v1",
                   f"{work_dir}/sm_{epoch}", f"{records_dir}/records_{epoch}.bin", "--games", f"{args.games}"]
        if args.batch_size:
            playout_args.extend(["--batch_size", args.batch_size])
        if args.playout_limit:
            playout_args.extend(["--playout_limit", args.playout_limit])
        check_call(playout_args, records_dir / f"records_{epoch}.bin")
        train_args = ["python", "-m", "othello_train.rl_train_v1", f"{work_dir}/cp_{epoch}/cp", f"{work_dir}/cp_{epoch+1}/cp",
                   f"{records_dir}/records_{epoch}.bin", "--epoch", f"{args.train_epoch}"] + model_args
        check_call(train_args, work_dir / f"cp_{epoch+1}")
        check_call(["python", "-m", "othello_train.checkpoint_to_savedmodel_v1",
                   f"{work_dir}/cp_{epoch+1}/cp", f"{work_dir}/sm_{epoch+1}"] + model_args, work_dir / f"sm_{epoch+1}")


if __name__ == "__main__":
    main()
