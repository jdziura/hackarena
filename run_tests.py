# Example usage:
# python3 run_tests.py -n 20 -b random_bot_v1 random_bot_v1 -r --ticks 500 --broadcast-interval 200 --grid-dimension 8
import os
import subprocess
import argparse
from concurrent.futures import ThreadPoolExecutor
import socket
import time
import datetime
import json
from collections import defaultdict


def find_available_port(base_port, used_ports):
    """Finds the next available port starting from base_port that is not in used_ports."""
    port = base_port
    while port in used_ports or not is_port_available(port):
        port += 1
    used_ports.add(port)
    return port


def is_port_available(port):
    """Check if a port is available."""
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        result = sock.connect_ex(("localhost", port))
        return result != 0


def run_server(test_id, port, num_players, args, experiment_dir):
    replay_filepath = os.path.join(experiment_dir, f"replay_{test_id}.json")
    command = [
        "../linux-x64/GameServer",
        "--host",
        args.host,
        "--port",
        str(port),
        "--number-of-players",
        str(num_players),
        "--grid-dimension",
        str(args.grid_dimension),
        "--ticks",
        str(args.ticks),
        "--broadcast-interval",
        str(args.broadcast_interval),
        "--match-name",
        f"match_{test_id}",
    ]
    if args.sandbox:
        command.append("--sandbox")
    if args.replay:
        command.extend(
            [
                "--save-replay",
                "--replay-filepath",
                replay_filepath,
                "--overwrite-replay-file",
            ]
        )

    print(
        f"[INFO] Starting server for test {test_id} on port {port} with {num_players} players"
    )
    print(f"[COMMAND] {' '.join(command)}")
    subprocess.Popen(command)
    time.sleep(10)  # Wait for 10 seconds to allow the server to initialize properly


def run_bot(test_id, bot_name, bot_id, port, host):
    unique_nickname = f"bot_{test_id}_{bot_id}"
    command = [
        f"./bot_binaries/{bot_name}",
        "--nickname",
        unique_nickname,
        "--host",
        host,
        "--port",
        str(port),
    ]
    print(
        f"[INFO] Starting bot {bot_id} ({bot_name}) with nickname {unique_nickname} for test {test_id} on port {port}"
    )
    print(f"[COMMAND] {' '.join(command)}")
    subprocess.Popen(command)


def summarize_results(experiment_dir, num_tests):
    summary = defaultdict(int)
    for i in range(num_tests):
        replay_file = os.path.join(experiment_dir, f"replay_{i}.json")
        if os.path.exists(replay_file):
            try:
                with open(replay_file, "r") as file:
                    if os.stat(replay_file).st_size == 0:
                        print(f"[WARNING] Replay file {replay_file} is empty.")
                        continue

                    replay_data = json.load(file)
                    players = replay_data.get("players", [])
                    winner = max(players, key=lambda x: x.get("score", 0), default=None)
                    if winner:
                        winner_nickname = winner.get("nickname")
                        summary[winner_nickname] += 1
                    else:
                        print(f"[INFO] No winner found in replay file {replay_file}.")
            except json.JSONDecodeError:
                print(f"[ERROR] Failed to decode JSON in replay file {replay_file}.")
            except Exception as e:
                print(f"[ERROR] An unexpected error occurred: {e}")

    print("\n=== Experiment Summary ===")
    for nickname, wins in summary.items():
        print(f"{nickname}: {wins} wins")
    print("=========================")


def cleanup_processes():
    """Kills all GameServer and bot processes."""
    try:
        # Kill all GameServer processes
        subprocess.run(["pkill", "-f", "GameServer"], check=True)
        # Kill all bot processes
        subprocess.run(["pkill", "-f", "bot_binaries"], check=True)
        print("[INFO] All GameServer and bot processes have been terminated.")
    except subprocess.CalledProcessError as e:
        print(f"[ERROR] Failed to kill processes: {e}")


def main():
    parser = argparse.ArgumentParser(description="Run parallel game tests.")
    parser.add_argument("-n", type=int, default=100, help="Number of tests to run")
    parser.add_argument(
        "-b",
        "--bots",
        nargs="+",
        required=True,
        help="List of bot binaries to use (up to 4)",
    )
    parser.add_argument(
        "--host", default="localhost", help="Host address for the game servers"
    )
    parser.add_argument(
        "--grid-dimension", type=int, default=24, help="Grid dimension for the game"
    )
    parser.add_argument(
        "--ticks", type=int, default=3000, help="Number of ticks per game"
    )
    parser.add_argument(
        "--broadcast-interval",
        type=int,
        default=100,
        help="Broadcast interval in milliseconds",
    )
    parser.add_argument(
        "-r", "--replay", action="store_true", help="Save replays of matches"
    )
    parser.add_argument(
        "--sandbox", action="store_true", help="Run servers in sandbox mode"
    )
    args = parser.parse_args()

    num_bots = len(args.bots)
    if num_bots > 4:
        print("[ERROR] You can only specify up to 4 bots.")
        return

    # Create a unique directory for this experiment
    timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    experiment_dir = os.path.join("./data", f"experiment_{timestamp}")
    os.makedirs(experiment_dir)

    used_ports = set()
    base_port = 5000

    # Use ThreadPoolExecutor to run servers and clients concurrently
    with ThreadPoolExecutor(max_workers=args.n * (num_bots + 1)) as executor:
        for i in range(args.n):
            port = find_available_port(base_port, used_ports)
            executor.submit(run_server, i, port, num_bots, args, experiment_dir)
            time.sleep(1)
            # Run bots for the server based on the number provided
            for bot_id, bot_name in enumerate(args.bots, start=1):
                executor.submit(run_bot, i, bot_name, bot_id, port, args.host)

    # Allow time for all experiments to finish
    total_time = args.broadcast_interval * args.ticks / 1000 + 15
    time.sleep(total_time)

    # Summarize results after all experiments are done
    summarize_results(experiment_dir, args.n)

    # Cleanup processes after summarizing
    cleanup_processes()


if __name__ == "__main__":
    main()
