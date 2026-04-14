import AppLogo from "../components/AppLogo";
import Button from "../components/Button";

export default function Onboarding() {
    return (
        <main className="flex h-screen w-screen items-center flex-col">
            <AppLogo className="w-28 h-28 mt-20"></AppLogo>
            <h2 className="font-bold text-3xl mt-7">Welcome to</h2>
            <h1 className="font-bold text-4xl mt-2">Atlas Engine</h1>
            <div className="mt-auto font-manrope font-bold ">
                <h1>by neutral software</h1>
            </div>
            <div className="mt-auto">
                <Button
                    type="primary"
                    onClick={() => {}}
                    className="shadow-2xl mb-10"
                >
                    Get started
                </Button>
            </div>
        </main>
    );
}
