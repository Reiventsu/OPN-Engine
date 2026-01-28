module;



// For now not used but in the future I may wanna decouple ImGui into its own thing

export module opn.System.Services.ImGui;
import opn.System.ServiceInterface;

export namespace opn {
    class ImGuiService final : public Service< ImGuiService > {
    protected:
        void onInit() override;

        void onShutdown() override;

        void onPostInit() override;

        void onUpdate( float _deltaTime ) override;
    };
}
