from worker_thread import WorkerThread


class sstorm(WorkerThread):
    def start(self):
        self.config.image_name = "ghcr.io/oran-testing/sstorm"
        self.cleanup_old_containers()
        self.setup_env()
        self.setup_networks()
        self.config.container_volumes[self.config.config_file] = {"bind": "/ue.conf", "mode": "ro"}
        self.setup_volumes()

        self.start_container()



