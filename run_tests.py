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
        "--eager-broadcast",
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

    log_file = os.path.join(experiment_dir, f"server_{test_id}.log")
    with open(log_file, "w") as log:
        print(
            f"[INFO] Starting server for test {test_id} on port {port} with {num_players} players"
        )
        print(f"[COMMAND] {' '.join(command)}")
        subprocess.Popen(command, stdout=log, stderr=log)
    time.sleep(10)  # Wait for 10 seconds to allow the server to initialize properly


def run_bot(test_id, bot_name, bot_id, port, host, experiment_dir):
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

    log_file = os.path.join(experiment_dir, f"bot_{test_id}_{bot_id}.log")
    with open(log_file, "w") as log:
        print(
            f"[INFO] Starting bot {bot_id} ({bot_name}) with nickname {unique_nickname} for test {test_id} on port {port}"
        )
        print(f"[COMMAND] {' '.join(command)}")
        # subprocess.Popen(command, stdout=log, stderr=log)
        subprocess.run(command, stdout=log, stderr=log)


def summarize_results(experiment_dir, num_tests, num_players):
    scores = defaultdict(int)
    draws = 0
    fails = 0
    for i in range(num_tests):
        replay_file = os.path.join(experiment_dir, f"replay_{i}_results.json")
        if os.path.exists(replay_file):
            try:
                with open(replay_file, "r") as file:
                    if os.stat(replay_file).st_size == 0:
                        print(f"[WARNING] Replay file {replay_file} is empty.")
                        fails += 1
                        continue

                    replay_data = json.load(file)
                    players = replay_data.get("players", [])

                    # Sort players based on score and kills
                    players.sort(key=lambda p: (p["score"], p["kills"]), reverse=True)

                    # Check for a draw if all players have the same score and kills
                    if len(players) > 0 and all(
                        p["score"] == players[0]["score"]
                        and p["kills"] == players[0]["kills"]
                        for p in players
                    ):
                        draws += 1
                    else:
                        # Allocate points based on the number of players
                        points_distribution = {4: [3, 2, 1, 0], 3: [2, 1, 0], 2: [1, 0]}
                        points = points_distribution.get(num_players)

                        if points:
                            for idx, player in enumerate(players):
                                nickname = player.get("nickname")
                                scores[nickname] += points[idx]
                        else:
                            print(f"[ERROR] Invalid number of players: {num_players}")

            except json.JSONDecodeError:
                print(f"[ERROR] Failed to decode JSON in replay file {replay_file}.")
            except Exception as e:
                print(f"[ERROR] An unexpected error occurred: {e}")

    # Summarize and print results
    print("\n=== Experiment Summary ===")
    points = [0 for _ in range(num_players)]
    for player, score in scores.items():
        player_number = int(player.split("_")[-1]) - 1  # Extract player index (1-based)
        if 0 <= player_number < num_players:
            points[player_number] += score

    for i, point in enumerate(points):
        print(f"Player {i + 1}: {point} points")
    print(f"Draws: {draws} matches")
    print(f"Failed matches: {fails}")
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
    base_port = find_available_port(5000, used_ports)

    batch_spread = 4
    for batch_start in range(0, args.n, batch_spread):
        futures = []
        with ThreadPoolExecutor(max_workers=batch_spread * (num_bots + 1)) as executor:
            # Limit to batch_spread or remaining tests, whichever is smaller
            current_batch_size = min(batch_spread, args.n - batch_start)

            test_ids = []
            ports = []

            for i in range(current_batch_size):
                test_id = batch_start + i
                port = find_available_port(base_port, used_ports)
                test_ids.append(test_id)
                ports.append(port)

            for i in range(current_batch_size):
                executor.submit(
                    run_server, test_ids[i], ports[i], num_bots, args, experiment_dir
                )

            time.sleep(1)
            print("[INFO] Waiting for 1 second before starting bots...")

            for i in range(current_batch_size):
                for bot_id, bot_name in enumerate(args.bots, start=1):
                    futures.append(
                        executor.submit(
                            run_bot,
                            test_ids[i],
                            bot_name,
                            bot_id,
                            ports[i],
                            args.host,
                            experiment_dir,
                        )
                    )

                # total_time = args.broadcast_interval * args.ticks / 1000 + 5
                # total_time = 10
                # print(f"[INFO] Waiting for {total_time} seconds before the next batch...")
                # time.sleep(total_time)
        
        print("[INFO] Waiting for games to complete...")
        for future in futures:
            future.result()

    # Summarize results after all experiments are done
    summarize_results(experiment_dir, args.n, len(args.bots))

    # Cleanup processes after summarizing
    cleanup_processes()


if __name__ == "__main__":
    main()
