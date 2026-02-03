module;

export module opn.Systems.ECS;
import opn.System.ServiceInterface;

export namespace opn {
    class ECS final : public Service< ECS > {
    protected:
        void onInit() override {
        }

        void onShutdown() override {
        }

        void onPostInit() override {
        }

        void onUpdate(const float _deltaTime) override {
        }
    };
}
